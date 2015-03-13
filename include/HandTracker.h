#pragma once
#include <opencv2/opencv.hpp>
#include "WindowManager.h"
#include "ColorSegmentation.h"
namespace iez {

class HandTracker {
public:
	HandTracker();
	void process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId = 0);

	// static methods
	void distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT);
	void findHandCenter(const cv::Mat &handDT, cv::Point &maxDTPoint);
	float findHandCenterRadius(const cv::Mat &binaryHandFiltered, const cv::Point &maxDTPoint, const std::vector<cv::Point> contour);

private:

};

}
