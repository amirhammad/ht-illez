#pragma once
#include <opencv2/opencv.hpp>
namespace iez {
class CHandTracker {
public:
	void findHandFromCenter(const cv::Mat &bgr, const cv::Mat &depth);
private:
	cv::Point3f interpolateToColor(const cv::Mat &bgr, cv::Point2f &coord);
	float calculateGradientInDirection(const cv::Point3f &prevColor,
		const cv::Point3f &nextColor);
};

};
