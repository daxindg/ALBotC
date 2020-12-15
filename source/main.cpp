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

extern std::unordered_map<std::string, std::unordered_map<std::string, cv::Mat>> features;
extern std::unordered_map<std::string, std::function<void(Pos)>> processer;
extern std::vector<std::string> events;
json config;



int should_exit = 0;
void config_init();
void init(int argc, char** argv) {
#ifdef _MSC_VER
    string path = R"(D:\Projects\ALBotC/config.json)";

#else
    string path = "/home/daxindg/data/Projects/ALBotC/config.json";
#endif
    if (argc > 1) path = argv[1];
    ifstream ifs(path);
    config = json::parse(ifs);
    config_init();
    ifs.close();

    for (auto &p : std::filesystem::recursive_directory_iterator(config["featuresDir"].get<string>())) {
  
        if (p.is_directory()) continue;
        auto fi = p.path().parent_path().filename().string();
        //fi = fi.substr(fi.find_last_of('/') + 1);
        auto se = p.path().filename().string();
        se = se.substr(0, se.find_last_of('.'));
        features[fi][se] = cv::imread(p.path().string(), cv::IMREAD_COLOR);
        LOGD("feature loaded %s %s", fi.c_str(), se.c_str());
    }

    signal(SIGINT, [](int){
        if (!config["combat"]["enabled"]) should_exit++;
        if (should_exit == 0) {
            LOGI("Keyboard Interrupted, wait for current combat end");
        }
        should_exit++;
    });
}


int wtfcnt = 0;
std::pair<Pos, std::string> scan() {
    Screen::getInstance().update();

    // for (auto k1 : {"message", "at"}) {
    //     for (auto &[k2, val] : features[k1]) {
    for (auto &k2 : events) {
        cv::Mat val;
        for (auto k1 : {"message", "at"}) {
            if (features[k1].find(k2) != features[k1].end()) {
                val = features[k1][k2];
                break;
            }
        }
        auto pos = Screen::find(val, 0);
        if (pos.first == 0) continue;
        wtfcnt = 0;
        return {pos, k2};
    }
    wtfcnt++;
    if (wtfcnt > 50) exit(0);
    return {{0, 0}, "nothing_found"};
}

void proc(std::pair<Pos, std::string> ps) {
    auto [p, s] = ps;

    LOGI(" proc > %s.", s.c_str());
    
    
    if (should_exit == 1 && s == "stage_info") {
        exit(0);
    }
    else if (should_exit >= 2) {
        LOGI("Keyboard Interrupted, exiting.");
        exit(0);
    }
    processer[s](p);
}

void loop() {
    // for (auto k1 : {"message", "at"}) {
    //     for (auto &[k2, val] : features[k1]) {
    //         std::cout << k2 << std::endl;
    //     }
    // }
    while (true) {
        proc(scan());
        sleep(2s);
    }
}

int main(int argc, char** argv) {

    init(argc, argv);

    loop();
    cin.get();
    return 0;
}
