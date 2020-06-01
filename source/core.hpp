#ifndef CORE_HPP
#define CORE_HPP 1

#include <unordered_map>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "utils.hpp"

#define FEATURE_MATCH 0
#define TEMPLATE_MATCH 1

class Screen {
  private:
    cv::Mat screen;

    Screen();
    ~Screen();
    Screen(const Screen &) = delete;
    Screen & operator=(const Screen &) = delete;

  public:
    static Screen& getInstance();
    static cv::Mat& getScreen(int screenColor=cv::IMREAD_COLOR, int forceUpdate=1);
    static void touch(Pos p);
    static void swipe(Pos p1, Pos p2, int duration=300);
    static Pos find(cv::Mat feature, int forceUpdate=1, double treshold=0.95, int method=TEMPLATE_MATCH);
    static std::vector<Pos> findAll(cv::Mat feature, int forceUpdate=1, int method=FEATURE_MATCH);

    Screen & update(int screenColor=cv::IMREAD_COLOR);
};




#endif