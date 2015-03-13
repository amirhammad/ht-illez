#include "HandTracker.h"
#include <assert.h>
#include <vector>

#include "Processing.h"

namespace iez {

void HandTracker::distanceTransform(const cv::Mat &binaryHand, cv::Mat &binaryHandFiltered, cv::Mat &handDT)
{
	cv::medianBlur(binaryHand, binaryHandFiltered, 13);

	cv::distanceTransform(binaryHandFiltered, handDT, CV_DIST_L2, 3);

	cv::Mat gray;
	handDT.convertTo(gray, CV_8UC1, 10.0f);
	WindowManager::getInstance().imShow("distanceTransform", gray);

}

void HandTracker::findHandCenter(const cv::Mat &handDT,	 cv::Point &maxDTPoint)
{
	maxDTPoint = cv::Point(-1, -1);
	float max = 0;
	for (int i = 0; i < handDT.rows; i++) {
		for (int j = 0; j < handDT.cols; j++) {
			const float d = handDT.at<float>(i, j);
			if (d > max) {
				max = d;
				maxDTPoint = cv::Point(j, i);
			}
		}
	}
}

float HandTracker::findHandCenterRadius(const cv::Mat& binaryHandFiltered,
		const cv::Point& maxDTPoint)
{
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(binaryHandFiltered, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	float minDistance = binaryHandFiltered.rows*binaryHandFiltered.cols;

	if (contours.size() == 0) {
		return 0;
	}
	for (int i = 0; i < contours[0].size(); i++) {
		float dist = Processing::pointDistance(maxDTPoint, contours[0][i]);
		if ( dist < minDistance) {
			minDistance = dist;
		}
	}
	return minDistance;
}

HandTracker::HandTracker()
{

}

void HandTracker::process(const cv::Mat &bgr, const cv::Mat &depth)
{

	// palm highlighting
	const cv::Mat &binaryHand = Processing::filterDepth2(depth);

	cv::Mat handDT;
	cv::Mat binaryHandFiltered;
	distanceTransform(binaryHand, binaryHandFiltered, handDT);

	cv::Point handCenter;
	findHandCenter(handDT, handCenter);
	float innerCircleRadius = findHandCenterRadius(binaryHandFiltered, handCenter);

	if (innerCircleRadius <= 0) {
		return;
	}

	cv::Mat centerHighlited = bgr.clone();

	cv::ellipse(centerHighlited, handCenter, cv::Size(innerCircleRadius, innerCircleRadius), 0, 0, 360, cv::Scalar(100, 0, 255), 10);
	WindowManager::getInstance().imShow("handCenter", centerHighlited);

}

} // Namespace iez
