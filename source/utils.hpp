#ifndef UTILS_HPP
#define UTILS_HPP 1

#include <utility>
#include <thread>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>


#define sleep(t) std::this_thread::sleep_for(t)

struct Pos: std::pair<int, int> {
    Pos(int fi=0, int se=0):std::pair<int, int>(fi, se){}
    Pos(cv::Point pt):std::pair<int, int>(pt.x, pt.y){}
    cv::Point toCvPoint() {
        return cv::Point(first, second);
    }
    Pos & tap();
    Pos & randomTap(int maxOffset=20);
};

Pos operator+(Pos a, int b);

std::vector<char>
_exec(const char* cmd);

std::vector<char>
_exec(std::string cmd);

std::string
exec(const char* cmd);

std::string
exec(std::string cmd);

template<typename T>
void
mergePoints(T &points, int minAcceptableCount=1,bool sorted=false, int maxDistance=100);

Pos
templateMatch(cv::Mat templ, cv::Mat source, double threshold=0.90);

std::vector<Pos>
templateMatchAll(cv::Mat templ, cv::Mat source, double threshold=0.90);

Pos
featureMatch(cv::Mat &feature, cv::Mat &source,  int minHessian=400, float ratio_thresh=0.7);

std::vector<Pos>
featureMatchAll(cv::Mat feature, cv::Mat source);

#endif