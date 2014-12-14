#include "HandTracker.h"
#include <assert.h>
#include <vector>
#include <eigen3/Eigen/Eigen>

using namespace iez;
using namespace cv;
using namespace std;
void CHandTracker::findHandFromCenter(const cv::Mat& bgr, const cv::Mat& depth)
{
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	int rows = bgr.rows;
	int cols = bgr.cols;

	Point2i center(cols/2,rows/2);
	Mat hand;
//	hand.create(bgr.rows, bgr.cols, CV_8UC1);
	cvtColor(bgr, hand, COLOR_BGR2GRAY);
//	bgr.copyTo(hand);

	for (float angle = 0; angle < 2*M_PI; angle += M_PI/9) {
		const float step_c = 1;
		const float maxHandSize_c = 200; //in pixels
		float dx = cos(angle);
		float dy = sin(angle);

		float sum = 0;
		int i;
		for (float step = step_c, i = 1; step<maxHandSize_c; step+=step_c, ++i) {

			Point2f currPoint(center.x+dx*step, center.y+dy*step);
			Point2f prevPoint(center.x+dx*(step-1), center.y+dy*(step-1));
			Point2f nextPoint(center.x+dx*(step+1), center.y+dy*(step+1));
			// calculate gradient

			// if current gradient in that direction is bigger than threshold, save border point
//			Point3_<uint8_t> currColor = interpolateToColor(bgr, currPoint);
			const Point3f prevColor = interpolateToColor(bgr, prevPoint);
			const Point3f nextColor = interpolateToColor(bgr, nextPoint);
			const float gradient = calculateGradientInDirection(prevColor, nextColor);

			hand.at<uint8_t>(currPoint.y, currPoint.x) = static_cast<uint8_t>(gradient);
		}
		imshow("hand", hand);
	}
	Eigen::Matrix<int,2,2> x;
}

cv::Point3f CHandTracker::interpolateToColor(const cv::Mat &bgr, cv::Point2f &coord)
{


	const int c11 = static_cast<int>(coord.x);
	const int c12 = c11+1;
	const int c21 = static_cast<int>(coord.y);
	const int c22 = c21+1;

	const Point3_<uint8_t> p11 = bgr.at<Point3_<uint8_t>>(c11,c21);
	const Point3_<uint8_t> p12 = bgr.at<Point3_<uint8_t>>(c11,c22);
	const Point3_<uint8_t> p21 = bgr.at<Point3_<uint8_t>>(c12,c21);
	const Point3_<uint8_t> p22 = bgr.at<Point3_<uint8_t>>(c12,c22);

	const float w11 = coord.x-static_cast<float>(c11);
	const float w12 = 1-w11;
	const float w21 = coord.y-static_cast<float>(c21);
	const float w22 = 1-w21;

	const Point3f colorPixel =  p11*w11 + p12*w12
								+ p21*w21 + p22*w22;

	return colorPixel;
}

float iez::CHandTracker::calculateGradientInDirection(
		const cv::Point3f &prevColor,
		const cv::Point3f &nextColor)
{
	Point3f diffColor(static_cast<int>(nextColor.x) - static_cast<int>(prevColor.x),
					static_cast<int>(nextColor.y) - static_cast<int>(prevColor.y),
					static_cast<int>(nextColor.z) - static_cast<int>(prevColor.z));

	return sqrt(diffColor.dot(diffColor));
//	return sqrt(diffColor.x*diffColor.x + diffColor.y*diffColor.y + diffColor.z*diffColor.z);
}
