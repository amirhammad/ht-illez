#pragma once
#include <opencv2/opencv.hpp>
namespace iez {

class Util {
public:
	static float pointDistance(const cv::Point &pt1, const cv::Point &pt2);
	static cv::Point pointMean(const cv::Point &pt1, const cv::Point &pt2, const float ratio12 = 0.5f);
	static void filterDepth(cv::Mat &dst, const cv::Mat &src, int near = -1, int far = -1);
	static cv::Mat filterDepthMask(const cv::Mat &src, int near = -1, int far = -1);
	static int findMin(const cv::Mat &depth);
	static int findMin2(const cv::Mat &depth, cv::Point &point);
	static float findMax(const cv::Mat &mat);
	static float findMax2(const cv::Mat &mat, cv::Point &point);
	static cv::Point calculateMean(const std::vector<cv::Point>&);
	static cv::Point findNearestPoint(const std::vector<cv::Point> &pointVector, const cv::Point refPoint);
	static void rotate(cv::Mat& src, double angle, cv::Mat& dst);
	static cv::Point calculateWeightedMean(const std::vector<cv::Point>&);
	static cv::Point calculateMeanIndices(const cv::Mat&);
};
}
