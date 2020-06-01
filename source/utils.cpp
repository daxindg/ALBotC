#include <algorithm>
#include <iostream>
#include <random>

#include <opencv2/xfeatures2d.hpp>

#include "log_def.h"
#include "utils.hpp"

Pos operator+(Pos a, int b) {
    return Pos(a.first + b, a.second + b);
}
Pos & Pos::tap() {
    extern const char deviceType;
    char cmd[100]{0};
    std::snprintf(cmd, sizeof cmd, "adb -%c shell input tap %d %d", deviceType, first, second);
    LOGD("      result of cmd (%s) > %s", cmd, exec(cmd).c_str());
    return *this;
}

Pos & Pos::randomTap(int maxOffset) {
    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<int> offset(-maxOffset, maxOffset);
    int of = first, os = second;
    first += offset(e);
    second += offset(e);
    tap();
    first =of, second = os;
    return *this;
}

std::vector<char> _exec(const char* cmd) {
    std::array<char, 256> buffer;
    std::vector<char> res;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        LOGE("          Failed to execute command : %s", cmd);
        // throw;
    }
    else {
        size_t sz = 0;
        while ((sz = fread(buffer.data(), sizeof buffer[0], buffer.size(), pipe.get()))) {
            std::move(buffer.begin(), buffer.begin() + sz, std::back_inserter(res));
        }
    }
    return res;
}

std::vector<char> _exec(std::string cmd) {
    return _exec(cmd.c_str());
}

std::string exec(const char* cmd) {
    auto res = _exec(cmd);
    return std::string(res.begin(), res.end());
}

std::string exec(std::string cmd) {
    return exec(cmd.c_str());
}



Pos
templateMatch(cv::Mat templ, cv::Mat source, double threshold) {
    int rows = templ.rows, cols = templ.cols;
    cv::Mat result;
    cv::matchTemplate(source, templ, result, cv::TM_CCOEFF_NORMED);

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    if (maxVal < threshold) return {0, 0};
    return Pos(maxLoc.x + cols / 2, maxLoc.y + rows / 2);
}

std::vector<Pos>
templateMatchAll(cv::Mat templ, cv::Mat source, double threshold) {
    
    cv::Mat result;
    cv::matchTemplate(source, templ, result, cv::TM_CCOEFF_NORMED);

    int channels = result.channels();
    int rows = result.rows;
    int cols = result.cols * channels;
    if (result.isContinuous()) {
        cols *= rows;
        rows = 1;
    }

    std::vector<Pos> ret;

    for (int i = 0; i < rows; i++) {
        auto p = result.ptr<float>(i);
        for (int j = 0; j < cols; j++) {
            if (p[j] >= threshold) ret.emplace_back((i * rows + j) / channels % result.cols, (i * rows + j) / channels / result.cols);
        }
    }
    mergePoints(ret);
    return ret;
}

Pos
featureMatch(cv::Mat &feature, cv::Mat &source, int minHessian, float ratio_thresh) {

    auto detector = cv::xfeatures2d::SURF::create(minHessian);
    std::vector<cv::KeyPoint> featureKeyPoints, sourceKeyPoints;
    cv::Mat featureDescriptors, sourceDescriptors;
    detector -> detectAndCompute(feature, cv::noArray(), featureKeyPoints, featureDescriptors);
    detector -> detectAndCompute(source, cv::noArray(), sourceKeyPoints, sourceDescriptors);
    
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    std::vector<std::vector<cv::DMatch>> knn_matches;
    matcher->knnMatch(featureDescriptors, sourceDescriptors, knn_matches, 2);

    std::vector<cv::DMatch> good_matches;
    for (size_t i = 0; i < knn_matches.size(); i++) {
        if (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance) {
            good_matches.emplace_back(knn_matches[i][0]);
        }
    }

    std::vector<Pos> results;
    for (auto &it : good_matches) {
        results.emplace_back(sourceKeyPoints[it.trainIdx].pt.x, sourceKeyPoints[it.trainIdx].pt.y);
    }
    mergePoints(results, 3);

    LOGD("          In function featureMatch: Matched %lu point(s).", results.size());

    if (results.empty()) return {0, 0};
    return results[0];
}

std::vector<Pos>
featureMatchAll(cv::Mat feature, cv::Mat source) {
    std::vector<Pos> res;
    Pos pos(0, 0);
    while ((pos = featureMatch(feature, source)).first) {
        res.emplace_back(pos);
        cv::rectangle(source, (pos + (-50)).toCvPoint(), (pos + 100).toCvPoint(), cv::Scalar(255, 255, 255), -1);
        cv::circle(source, pos.toCvPoint(), 5, cv::Scalar(0, 0, 0));
    }
    mergePoints(res);
    LOGD("          In funnction featureMatchAll: Matched %lu point(s).", res.size());

    return res;
}

template<typename T>
void mergePoints(T & points, int minAcceptableCount,bool sorted, int maxDistance) {

    static auto square = [](int s) {return s * s;};
    static auto dis = [](Pos p1, Pos p2) {
        return square(p1.first - p2.first) + square(p1.second - p2.second);
    };
    maxDistance *= maxDistance;
    int sz = 0;
    if (!sorted) std::sort(points.begin(), points.end());
    std::vector<std::pair<int, Pos>> tmp;
    for (size_t i = 0; i < points.size(); i++) {
        if (points[i].first == 0) continue;
        sz++;
        tmp.emplace_back(1, points[i]);
        for (auto j = i + 1; j < points.size(); j++) {
            if (points[j].first == 0) continue;
            if (dis(points[i], points[j]) < maxDistance) points[j].first = 0, tmp.back().first++;
        }
    }

    if (!sorted) std::sort(tmp.begin(), tmp.end(), std::greater<std::pair<int, Pos>>());
    points.clear();
    for (auto &[c, x] : tmp) {
        if (c < minAcceptableCount) break;
        points.emplace_back(x);
    }
}

#include <queue>

template void mergePoints<std::deque<Pos>>(std::deque<Pos> & points, int minAcceptableCount,bool sorted, int maxDistance);
template void mergePoints<std::vector<Pos>>(std::vector<Pos> & points, int minAcceptableCount, bool sorted, int maxDistance);