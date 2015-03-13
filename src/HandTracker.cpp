#include "HandTracker.h"
#include <assert.h>
#include <vector>
#include <limits>

#include "Processing.h"

namespace iez {

void HandTracker::distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT)
{
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
		const cv::Point& maxDTPoint, const std::vector<cv::Point> contour)
{
	float minDistance = binaryHandFiltered.rows*binaryHandFiltered.cols;

	for (int i = 0; i < contour.size(); i++) {
		float dist = Processing::pointDistance(maxDTPoint, contour[i]);
		if ( dist < minDistance) {
			minDistance = dist;
		}
	}
	return minDistance;
}

HandTracker::HandTracker()
{

}

#define NEXT_HAND_TOLERANCE (5)
#define HAND_MAX_PHYSICAL_DEPTH	(170)
#define C3	(90)
void HandTracker::process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId)
{

	/// Create binary hand

	cv::Mat binaryHand;
	std::vector<cv::Point> handContour;
	cv::Mat candidates;

	cv::Point referencePoint; // used for selection of hand from multiple components (nearest)

	bool isReferenceImage = true;


	int near;
	int far;
	if (isReferenceImage) {
		qDebug("%d referenceImage", imageId);
		// we had not image for long time...
		near = Processing::findMin2(depth, referencePoint);
		far = near + HAND_MAX_PHYSICAL_DEPTH;
	} else {
//		referencePoint = m_prevHandPosition;
//		near = m_prevHandDepth - NEXT_HAND_TOLERANCE;
//		far = near + HAND_MAX_PHYSICAL_DEPTH + NEXT_HAND_TOLERANCE;
	}

	candidates = Processing::filterDepth2(depth, near, far);

	// mask with rectangle
//	cv::Mat mask = cv::Mat::zeros(depth.rows, depth.cols, CV_8UC1);
//	cv::Point topLeft(m_prevHandPosition.x - C3, m_prevHandPosition.y - C3);
//	cv::Point bottomRight(m_prevHandPosition.x + C3, m_prevHandPosition.y + C3);

//	cv::rectangle(mask, topLeft, bottomRight, cv::Scalar_<uint8_t>(255), CV_FILLED);
//	cv::Mat f;
//	candidates.copyTo(f, mask);
//	f.copyTo(candidates);
//	WindowManager::getInstance().imShow("xxxx", candidates);


	/// Filter
	cv::Mat candidatesFiltered;
	cv::medianBlur(candidates, candidatesFiltered, 13);
//		cv::Mat tmp;
//		cv::erode(frontMostObjects, tmp, cv::Mat(), cv::Point(-1,-1), 2);// dilate main
//		cv::dilate(tmp, frontMostObjectsFiltered, cv::Mat(), cv::Point(-1,-1), 2);// dilate main

//		 fill
//		cv::floodFill(frontMostObjectsFiltered, frontMostPoint, cv::Scalar_<uint8_t>(50));

	/// Contours -> find single contour
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(candidatesFiltered, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	if (contours.size() == 0) {
		qDebug("%d Zero objects", imageId);

		if (!isReferenceImage) {
			process(bgr, depth, imageId);
		}
		return;
	} else if (contours.size() == 1) {
		handContour = contours[0];
	} else {
		float minDist = std::numeric_limits<float>::max();
		int minIndex = std::numeric_limits<int>::max();
		cv::Point mid;
		for (int i = 0; i < contours.size(); i++) {
			const cv::Point tmpmid = Processing::calculateMean(contours[i]);
			const float d = Processing::pointDistance(tmpmid, referencePoint);
			if (minDist > d) {
				minDist = d;
				minIndex = i;
				mid = tmpmid;
			}
		}
		handContour = contours[minIndex];

	}

	/// fill contour

	cv::Mat out = cv::Mat::zeros(depth.rows, depth.cols, CV_8UC1);
	std::vector<std::vector<cv::Point> > hc(1);
	hc[0] = handContour;
	cv::fillPoly(out, hc, cv::Scalar_<uint8_t>(255));

	if (isReferenceImage) {

	} else {
//		int minDepth = std::numeric_limits<int>::max();
//		for (int i = 0; i < out.rows; i++) {
//			for (int j = 0; j < out.cols; j++) {
//				if (out.at<uint8_t>(i, j)) {
//					int d = depth.at<uint16_t>(i, j);
//					if (d > near && minDepth > d) {
//						minDepth = d;
//					}
//				}
//			}
//		}
//		m_prevHandDepth = minDepth;

	}
	/// save binary hand
	out.copyTo(binaryHand);

	cv::ellipse(out, referencePoint, cv::Size(5, 5), 0, 0, 360, cv::Scalar_<uint8_t>(200), 2);
	WindowManager::getInstance().imShow("binaryHand", binaryHand);



	/// Distance Transform

	cv::Mat handDT;
	distanceTransform(binaryHand, handDT);

	/// Find hand center

	cv::Point handCenter;
	findHandCenter(handDT, handCenter);


	/// Find palm radius

	float innerCircleRadius = findHandCenterRadius(binaryHand, handCenter, handContour);

	if (innerCircleRadius <= 0) {
		return;
	}


	/// Draw

	cv::Mat centerHighlited = bgr.clone();

	cv::ellipse(centerHighlited, handCenter, cv::Size(innerCircleRadius, innerCircleRadius), 0, 0, 360, cv::Scalar(100, 0, 255), 10);
	WindowManager::getInstance().imShow("handCenter", centerHighlited);

}

} // Namespace iez
