#pragma once
#include <opencv2/opencv.hpp>
#include "WindowManager.h"
#include "ColorSegmentation.h"
namespace iez {

class CHandTracker {
public:
	CHandTracker();
	void findHandFromCenter(const cv::Mat &bgr, const cv::Mat &depth);
private:
	cv::Point3f interpolateToColor(const cv::Mat &bgr, const cv::Point2f &coord);
	float calculateGradientInDirection(const cv::Point3f &prevColor,
		const cv::Point3f &nextColor);
	void extend(cv::Mat &hand, const cv::Mat &img, cv::Point2f center, cv::Point3f meanColor, int depth);
	ColorSegmentation segmentation;
};

}
