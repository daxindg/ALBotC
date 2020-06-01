#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <csignal>
#include <chrono>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

#include "core.hpp"
#include "utils.hpp"
#include "log_def.h"

// using namespace cv;
using namespace std::chrono_literals;
using namespace std;

using json = nlohmann::json;

extern unordered_map<std::string, std::unordered_map<std::string, cv::Mat>> features;
extern unordered_map<std::string, std::unordered_map<std::string, std::function<void(Pos)>>> processer;

json config;



int _exit = 0;
void config_init();
void init() {

    ifstream ifs("/home/daxindg/data/Projects/AzurLane/ALBotC/config.json");
    config = json::parse(ifs);
    config_init();
    ifs.close();

    for (auto &p : std::filesystem::recursive_directory_iterator(config["featuresDir"])) {
  
        if (p.is_directory()) continue;
        auto fi = p.path().parent_path().filename().string();
        //fi = fi.substr(fi.find_last_of('/') + 1);
        auto se = p.path().filename().string();
        se = se.substr(0, se.find_last_of('.'));
        features[fi][se] = cv::imread(p.path().string(), cv::IMREAD_COLOR);
        LOGD("feature loaded %s %s", fi.c_str(), se.c_str());
    }

    signal(SIGINT, [](int){
        if (!config["combat"]["enabled"]) _exit++;
        if (_exit == 0) {
            LOGI("Keyboard Interrupted, wait for current combat end");
        }
        _exit++;
    });
}


int wtfcnt = 0;
std::pair<Pos, std::pair<std::string, std::string>> scan() {
    Screen::getInstance().update();

    for (auto k1 : {"message", "at"}) {
        for (auto &[k2, val] : features[k1]) {
            auto pos = Screen::find(val, 0);
            if (pos.first == 0) continue;
            wtfcnt = 0;
            return {pos, {k1, k2}};
        }
    }
    wtfcnt++;
    if (wtfcnt > 50) exit(0);
    return {{0, 0}, {"nothing", "found"}};
}

void proc(std::pair<Pos, std::pair<std::string, std::string>> ppss) {
    auto [p, pss] = ppss;

    auto [a, b] = pss;

    LOGI(" proc > %s %s .", a.c_str(), b.c_str());
    
    
    if (_exit == 1 && b == "stage_info") {
        exit(0);
    }
    else if (_exit >= 2) {
        LOGI("Keyboard Interrupted, exiting.");
        exit(0);
    }
    processer[a][b](p);
}

void loop() {
    while (true) {
        proc(scan());
        sleep(2s);
    }
}

int main() {

    init();

    loop();

    return 0;
}
