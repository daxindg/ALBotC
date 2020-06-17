#include <chrono>
#include <algorithm>
#include <queue>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

#include "core.hpp"
#include "log_def.h"

using namespace std::chrono_literals;
using namespace std;
using json = nlohmann::json;

extern json config;

// Program settings
char deviceType; // 'd' for usb device, 'e' for emulator
// Combat settings
bool enableCombat;
string stage;
vector<string> enemyList;
int defaultFleet;
int bossFleet;
int beforeBoss;
int withdrawAfter;

void config_init(){
    deviceType    = config["deviceType"].get<string>()[0];
    enableCombat  = config["combat"]["enabled"].get<bool>();
    stage         = config["combat"]["stage"].get<string>();
    enemyList     = config["combat"]["enemyList"].get<vector<string>>();
    defaultFleet  = config["combat"]["defaultFleet"].get<int>();
    bossFleet     = config["combat"]["bossFleet"].get<int>();
    beforeBoss    = config["combat"]["beforeBoss"].get<int>();
    withdrawAfter = config["combat"]["withdrawAfter"].get<int>();
}

// /*      Class Screen        */

Screen::Screen(){}

Screen::~Screen() {}

Screen&
Screen::getInstance() {
    static Screen instance;
    return instance;
}

cv::Mat &
Screen::getScreen(int screenColor, int forceUpdate) {
    Screen & scr = Screen::getInstance();
    if (forceUpdate || scr.screen.empty()) scr.update(screenColor);

    return scr.screen;
}

Screen &
Screen::update(int screenColor) {
    char cmd[100]{0};
    std::snprintf(cmd, sizeof cmd, "adb -%c shell screencap -p", deviceType);
    screen = cv::imdecode(_exec(cmd),  screenColor);
    return *this;
}


void
Screen::swipe(Pos p1, Pos p2, int duration){
    char cmd[100]{0};
    std::snprintf(cmd, sizeof cmd, "adb -%c shell input swipe %d %d %d %d %d", deviceType, p1.first, p1.second, p2.first, p2.second, duration);
    exec(cmd);
}

Pos
Screen::find(cv::Mat feature, int forceUpdate, double threshold, int method) {
    Pos res{0, 0};
    if (method == FEATURE_MATCH) {
        res = featureMatch(feature, Screen::getScreen(cv::IMREAD_COLOR, forceUpdate));
    }
    else if (method == TEMPLATE_MATCH) {
        res = templateMatch(feature, Screen::getScreen(cv::IMREAD_COLOR, forceUpdate), threshold);
    }
    else {
        LOGE("Screen::find > Invalid match method, expect FEATURE_MATCH(0) TEMPLATE_MATCH(1) got (%d)", method);
    }
    return res;
}

std::vector<Pos>
Screen::findAll(cv::Mat feature, int forceUpdate, int method) {

    if (method == FEATURE_MATCH) {
        return featureMatchAll(feature, Screen::getScreen(cv::IMREAD_COLOR, forceUpdate));
    }
    else if (method == TEMPLATE_MATCH) {
        return templateMatchAll(feature, Screen::getScreen(cv::IMREAD_COLOR, forceUpdate));
    }

    LOGE("Screen::findAll > Invalid match method, expect FEATURE_MATCH(0) TEMPLATE_MATCH(1) got (%d)", method);
    return std::vector<Pos>();
}



struct State {
//temperary
#ifdef ALP
    int wtfCnt = 0;

#endif

//Combat
    int combatCount = 0;
    int battleCount = 0;
    int beforeBoss = 0;
    int moveFailCount = 0;
    std::deque<Pos> enemy{};
    int adjustCount = 0;
    int adjustCountAfterBossAppear = 0;


    bool bossAppear = false;
    bool stopCombat = false;
    bool inCombat = false;
    bool inBattle = false;

    void clear() {
        #ifdef ALP
        wtfCnt = 0;
        #endif
        battleCount = 0;
        moveFailCount = 0;
        enemy = {};
        adjustCount = 0;
        adjustCountAfterBossAppear = 0;

        bossAppear = false;
        stopCombat = false;
        inCombat = false;
        inBattle = false;
    }

    void print() {
        LOGI("combatCount: %d , battleCount: %d / %d, moveFailCount: %d, adjustConut: %d\n\
        inCombat: %d, inBattle: %d, stopCombat: %d", combatCount, battleCount, withdrawAfter, moveFailCount, adjustCount, inCombat, inBattle, stopCombat);
    }

} state;


enum kpNames{
left_navi, oil, gold,

nxt, back,

to_commission, last_commission, commission_advice, commission_start,

weigh_anchor_in_select_fleet, switch_fleet,

event_entrance,

filter, all, n, r, sr
};


std::unordered_map<kpNames, Pos> keyPoints = {
    {left_navi, Pos(25, 250)}, {oil, Pos(250, 145)}, {gold, Pos(625, 145)},
    
    {nxt, Pos(1650, 980)}, {back, Pos(70, 70)},

    {to_commission, Pos(665, 425)}, {last_commission, Pos(1200, 925)}, {commission_advice, Pos(1400, 530)}, {commission_start, Pos(1650, 530)},
    
    {weigh_anchor_in_select_fleet, Pos(1600, 910)}, {switch_fleet, Pos(1550, 1024)},

    {event_entrance, Pos(1800, 300)},

    {filter, Pos(1700, 40)}, {all, Pos(530, 690)}, {n, Pos(760, 690)}, {r, Pos(1000, 690)}, {sr, Pos(1240, 690)},
};



// enum Key1{
//     at,
//     message,

// };

void swipe(Pos o, Pos d, int duration=600) {
    char cmd[50];
    snprintf(cmd, sizeof cmd, "adb -%c shell input swipe %d %d %d %d %d", deviceType, o.first, o.second, o.first + d.first, o.second + d.second, duration);
    exec(cmd);
}

std::unordered_map<std::string, std::function<void(void)>> preAdjust{
    {"7-2", [](){swipe({428, 316},{1034, 406});}},
    {"8-2", [](){swipe({428, 316},{945, 294});}},


    {"E-C2", [](){swipe({428, 316},{1000, 431});}},
    {"E-C3", [](){swipe({428, 316},{659, 311});}},
    {"E-A3", [](){swipe({428, 316},{1199, 535});}},
    {"E-B1", [](){swipe({550, 310},{1100, 500});}},
    {"E-B3", [](){swipe({582, 292},{534, 550});}},
    {"E-SP3", [](){swipe({582, 292},{1600, 600});}},
    {"default", [](){
        swipe({428, 316},{515, 450});
    }}
};
std::unordered_map<std::string, std::vector<std::function<void(void)>>> adjust{
    {"E-C2", {
        [](){ swipe({960, 540}, {0, 300}); },
        [](){ swipe({960, 540}, {0, -300}); },
        [](){ swipe({960, 540}, {0, -300}); },
        [](){ swipe({960, 540}, {0, 300}); },
    }},
    {"default", {
        [](){ swipe({960, 540}, {0, 300}); },
        [](){ swipe({960, 540}, {0, -300}); },
        [](){ swipe({960, 540}, {0, -300}); },
        [](){ swipe({960, 540}, {0, 300}); },
    }}
};

std::unordered_map<std::string, std::unordered_map<std::string, cv::Mat>> features;


inline void adjustToDefaultPosition() {
    swipe({1800, 900}, {-1200, -700});
    swipe({1800, 900}, {-1200, -700});
    if (preAdjust.find(stage) != preAdjust.end()) {
        preAdjust[stage]();
    }
    else preAdjust["default"]();
}


std::vector<std::string> events{
"dock_full",
"inform",

"in_battle",
"before_battle",
"battle_end",
"exp_page",
"get_item",

"stage_info",
"map",
"select_fleet",
"combat",

"retire_bonus",
"retire",


"perfect",
"complete",
"commission_complete",
"commission_available",
"commission",
"skill_class",
"main",

"battle_failed",
"drop_r",
"drop_sr",
"drop_ssr",
};

std::unordered_map<std::string, std::function<void(Pos)>> processer{
    {"main", [](Pos pos){
        if (enableCombat) {
            state.inCombat = false;
            if (state.stopCombat) {
                state.stopCombat = false;
                keyPoints[left_navi].randomTap();
                return;
            }
            pos.randomTap();
        }
        else {
            LOGI("          Nothing to do, wate for 60s");
            sleep(60s);
            keyPoints[left_navi].randomTap();
        }
    }},

    {"commission", [](Pos) {
        if (Screen::find(features["symbol"]["no_commission_left"], 0).first) {
            LOGI("      All commissions running.");
            keyPoints[back].randomTap();
        }
        else {
            Screen::swipe({1700, 900}, {1700, 650});
            sleep(500ms);
            keyPoints[last_commission].randomTap();
            sleep(500ms);
            keyPoints[commission_advice].randomTap();
            sleep(800ms);
            keyPoints[commission_start].randomTap();
        }
    }},
    
    {"skill_class", [](Pos start){
        for (std::string k1 : {"t1", "t2", "t3"}) {
            for (std::string k2 : {"defense", "offense", "support"}) {
                Pos pos = Screen::find(features["book"][k1 + "_" + k2], 0);
                if (pos.first) {
                    pos.randomTap();
                    LOGI("      Use %s %s skillbook.", k1.c_str(), k2.c_str());
                    sleep(500ms);
                    start.randomTap();
                    return;
                }
            }
        }
        LOGE("わがらん。。");
    }},
    
    {"map", [](Pos) {
        state.print();
        if (!state.stopCombat && enableCombat) {
            if (stage[0] == 'E') keyPoints[event_entrance].randomTap(), sleep(1s);
            Pos pos;
            // while (true) {
                pos = Screen::find(features["stage"][stage], 1);
                // if (pos.first) break;
            if (pos.first == 0){
                LOGE("      Connot find stage %s, you may need navigate to it by hand.", stage.c_str());
                return;
                sleep(5s);
            }
            // }
            LOGI("      Successfully found stage %s, starting combat.", stage.c_str());
            pos.randomTap();
        }
        else {
            keyPoints[back].randomTap();
            sleep(2s);
        }
    }},
    
    {"stage_info", [](Pos pos) {
        pos.randomTap();
    }},
    
    {"select_fleet", [](Pos){
        //TODO Select fleet ...
        keyPoints[weigh_anchor_in_select_fleet].randomTap(30);
    }},

    {"combat", [](Pos withdraw){
        state.print();
        static bool f0 = false;
        if (!state.inCombat) { // init state , start combat

            state.clear();
            state.beforeBoss = beforeBoss;
            state.inCombat = true;
            state.combatCount++;
            if (state.combatCount % 2 == 0) state.stopCombat = true;

            if (defaultFleet != 1) {
                sleep(1s);
                keyPoints[switch_fleet].randomTap();
                sleep(500ms);
            }

            adjustToDefaultPosition();
            f0 = true;
        }
        
        if (state.battleCount >= withdrawAfter) { // reach max battle count.
            withdraw.randomTap();
            state.inCombat = false;
            return;
        }

        std::vector<std::string> _enemyList = enemyList;
        
        if (state.battleCount >= state.beforeBoss) {
            LOGI("      Boss appear, finding boss...");
            _enemyList.resize(1);
            
            if (!state.bossAppear) {
                state.bossAppear = true;
                if (defaultFleet != bossFleet) {  // switch to boss team
                    sleep(1s);
                    keyPoints[switch_fleet].randomTap();
                    sleep(1s);
                    adjustToDefaultPosition();
                }
            }
        }

        Screen::getInstance().update();
        // if (state.battleCount == 0 && stage == "E-C2" && f0) {
        //     state.enemy.emplace_back(1000, 460);
        // }
        // if (state.battleCount == 0 && stage == "E-A3"  && f0) {
        //     state.enemy.emplace_back(920, 143);
        //     state.enemy.emplace_back(826, 620);
        // }
        // if (state.battleCount == 0 && stage == "E-B1"  && f0) {
        //     state.enemy.emplace_back(920, 143);
        //     state.enemy.emplace_back(1260, 486);
        // }
        f0 = false;
        if (state.enemy.empty()) {
            for (std::string t : _enemyList) {
                std::vector<Pos> res = Screen::findAll(features["enemy"][t], 0);
                LOGD("      %s %lu.", t.c_str(), res.size());
            
                if (t == "question_mark") {
                    int l = res.size();
                    for (int i = 0; i < min(4, l); i++) {
                        res[i].randomTap();
                        sleep(700ms);
                    }
                    continue;
                }
                std::copy(res.begin(), res.end(), std::back_insert_iterator<std::deque<Pos>>(state.enemy));
            }

            mergePoints(state.enemy, 1, true);
            std::for_each(state.enemy.begin(), state.enemy.end(), [](Pos &p){p = p + 20;});
            state.enemy.resize(std::partition(state.enemy.begin(), state.enemy.end(), [](Pos p){return !(p.first < 845 && p.second < 240) && p.second > 100 && !(p.first >= 970 && p.second >= 945) && !(p.second < 814 && p.second > 583 && p.first > 1740) && p.first > 190;}) - state.enemy.begin());
        }

        if (state.enemy.empty()) {
            LOGW("      Cannot find enemy, adjusting...");
            if (state.adjustCount == 0) {

                adjustToDefaultPosition();
            }
            if (adjust.find(stage) != adjust.end()) {
                adjust[stage][state.adjustCount]();
            }
            else adjust["default"][state.adjustCount]();

            state.adjustCount++;
            state.adjustCount %= 4;

            
            if (state.battleCount >= state.beforeBoss) {
                state.adjustCountAfterBossAppear++;
                state.adjustCountAfterBossAppear %= 4;
                if (state.adjustCountAfterBossAppear == 0) { // Failed to find boss
                    state.beforeBoss++;
                    LOGW("      Connot find boss, start searching nornal enemy...");
                }
            }

            return;
        }

        
        LOGD("      to point (%d, %d)", state.enemy.front().first, state.enemy.front().second);
        state.enemy.front().randomTap();
        sleep(2s);

        if (state.moveFailCount > 3) {
            state.enemy.pop_front();
            state.moveFailCount = 0;
            if (state.battleCount >= state.beforeBoss) {
                state.beforeBoss++;
                LOGI("      Cannot reach boss, moving to another.");
            }
            return;
        }
        state.moveFailCount++;
    }},

    {"before_battle", [](Pos pos){
        auto p = Screen::find(features["button"]["auto_sub_off"], 0);
        if (p.first) p.randomTap();
        pos.randomTap();
        sleep(4s);
    }},

    {"in_battle", [](Pos){
        if (!state.inBattle) {
            state.battleCount++;
            state.inBattle = true;
            state.moveFailCount = 0;
            state.adjustCountAfterBossAppear = 0;
            state.enemy.clear();
            state.print();
        }

        auto pos = Screen::find(features["button"]["auto_battle"], 0);
        if (pos.first) pos.randomTap();

        sleep(8s);
    }},

    {"battle_end", [](Pos pos) {
        pos.randomTap();
        if (state.inBattle) { state.inBattle = false;
            if (state.battleCount > state.beforeBoss ) {  state.inCombat = false;
            }
        }
    }},
    {"battle_failed", [](Pos){
        LOGW("我裂开来");
        exit(0);            
    }},

    {"exp_page", [](Pos) {
        keyPoints[nxt].randomTap();
        sleep(5s);
    }},

    {"retire", [](Pos) {
        if (Screen::find(features["symbol"]["nothing_to_retire"]).first) {
            keyPoints[back].randomTap();
            sleep(2s);
            return;
        }

        keyPoints[filter].randomTap();
        sleep(500ms);
        keyPoints[all].randomTap(10);
        keyPoints[n].randomTap(10);
        keyPoints[r].randomTap(10);

        sleep(500ms);

        Pos(1240, 960).randomTap(); // Confirm filter
        sleep(500ms);

        for (auto it : {"n", "r"}) {
            auto p = Screen::findAll(features["ship"][it], 1, TEMPLATE_MATCH);
            std::for_each(p.begin(), p.end(), [](Pos p) {p.tap();});
        }
        sleep(500ms);

        Pos(1730, 1000).randomTap(); // Confirm retirement
    }},

    {"inform", [](Pos) {
        auto scared = Screen::find(features["symbol"]["oil"], 0, 0.85);
        if (scared.first) {
            Screen::find(features["button"]["close"], 0).randomTap();
            return;
        }
        scared = Screen::find(features["button"]["confirm"], 0);
        if (scared.first) scared.randomTap();
        else Screen::find(features["button"]["close"], 0).randomTap();
    }},

    {"complete", [](Pos pos){
        pos.randomTap();
    }},

    {"get_item", [](Pos) {
        keyPoints[nxt].randomTap();
    }},

    {"perfect", [](Pos) {
        keyPoints[nxt].randomTap();
    }},

    {"commission_complete", [](Pos){
        state.inCombat = false;
        keyPoints[left_navi].randomTap();
        std::this_thread::sleep_for(1s);
        keyPoints[oil].randomTap();
        sleep(300ms);
        keyPoints[gold].randomTap();
        sleep(300ms);
        keyPoints[to_commission].randomTap();

    }},

    {"commission_available", [](Pos p) {
        if (p.second > 540) {
            keyPoints[back].randomTap();
        }
        keyPoints[to_commission].randomTap();
    }},
    {"dock_full", [](Pos pos) {
        pos.randomTap();
    }},

    {"retire_bonus", [](Pos){
        Pos(1200, 855).randomTap();
    }},

    {"drop_r", [](Pos pos){pos.randomTap();}},
    {"drop_sr", [](Pos pos){pos.randomTap();}},
    {"drop_ssr", [](Pos pos){pos.randomTap();}},
    {"nothing_found", [](Pos){
        if (!state.inCombat) keyPoints[back].randomTap(5);
    }}
};

