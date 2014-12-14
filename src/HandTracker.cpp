#include "HandTracker.h"
#include <assert.h>
#include <vector>
#include <eigen3/Eigen/Eigen>
#include <armadillo>
#define MSG(x) std::cout<<x<<std::endl

using namespace cv;
//using namespace std;
namespace iez {

void CHandTracker::findHandFromCenter(const cv::Mat& bgr, const cv::Mat& depth)
{
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	int rows = bgr.rows;
	int cols = bgr.cols;

	Point2i center(cols/2,rows/2);
	Mat hand;
	Mat img;
//	img.create(bgr.rows, bgr.cols, CV_8UC3);
	medianBlur(bgr,img,11);
	imshow("img", img);
//	bgr.copyTo(img);

	cvtColor(bgr, hand, COLOR_BGR2GRAY);
//	cvtColor(img, img, COLOR_BGR2YCrCb);
//	bgr.copyTo(hand);
	Point3f meanColor = img.at<Point3_<uint8_t> >(center.y, center.x);
	extend(hand, img, center, meanColor, 0);
	imshow("hand", hand);
}

cv::Point3f CHandTracker::interpolateToColor(const cv::Mat &bgr, cv::Point2f &coord)
{


	const int c11 = static_cast<int>(coord.x);
	const int c12 = c11+1;
	const int c21 = static_cast<int>(coord.y);
	const int c22 = c21+1;

	const Point3_<uint8_t> p11 = bgr.at<Point3_<uint8_t> >(c21,c11);
	const Point3_<uint8_t> p12 = bgr.at<Point3_<uint8_t> >(c22,c11);
	const Point3_<uint8_t> p21 = bgr.at<Point3_<uint8_t> >(c21,c12);
	const Point3_<uint8_t> p22 = bgr.at<Point3_<uint8_t> >(c22,c12);

	const float w11 = coord.x-static_cast<float>(c11);
	const float w12 = 1-w11;
	const float w21 = coord.y-static_cast<float>(c21);
	const float w22 = 1-w21;

	const Point3f colorPixel =  p11*w11 + p12*w12
								+ p21*w21 + p22*w22;

	return colorPixel;
}

float CHandTracker::calculateGradientInDirection(
		const cv::Point3f &prevColor,
		const cv::Point3f &nextColor)
{
	Point3f diffColor(static_cast<int>(nextColor.x) - static_cast<int>(prevColor.x),
					static_cast<int>(nextColor.y) - static_cast<int>(prevColor.y),
					static_cast<int>(nextColor.z) - static_cast<int>(prevColor.z));

//	return sqrt(diffColor.dot(diffColor));
	return sqrt(diffColor.x*diffColor.x + diffColor.y*diffColor.y + diffColor.z*diffColor.z);
}

void CHandTracker::extend(cv::Mat& hand, const cv::Mat& img, cv::Point2f center,
		cv::Point3f meanColor, int depth)
{
	depth++;
	if (depth>20) {
		return;
	}
	if (hand.at<uint8_t>(center))
	for (float angle = 0; angle < 2*M_PI; angle += M_PI/5) {
		const float step_c = 1;
		const float maxHandSize_c = 50; //in pixels
		float dx = std::cos(angle);
		float dy = std::sin(angle);

		// center, img
		for (float step = step_c, i = 1; step<maxHandSize_c; step+=step_c, ++i) {

			Point2f currPoint(center.x+dx*step, center.y+dy*step);
			if (currPoint.x > img.cols
			|| currPoint.y > img.rows
			|| currPoint.x < 0
			|| currPoint.y < 0 ) {
				break;
			}
			Point2f prevPoint(center.x+dx*(step-1), center.y+dy*(step-1));
			Point2f nextPoint(center.x+dx*(step+1), center.y+dy*(step+1));
			// calculate gradient

			// if current gradient in that direction is bigger than threshold, save border point
			Point3f currColor = interpolateToColor(img, currPoint);
//			const Point3f prevColor = interpolateToColor(img, prevPoint);
//			const Point3f nextColor = interpolateToColor(img, nextPoint);

//			const float gradient = calculateGradientInDirection(prevColor, nextColor);
			const float gradient = calculateGradientInDirection(meanColor, currColor);


//			meanColor = (meanColor*i + currColor)/(i+1);
			hand.at<uint8_t>(currPoint.y, currPoint.x) = static_cast<uint8_t>(std::min<float>(255,gradient))*0 + 0;
			if (gradient > 200) {
				Point2f d(currPoint-center);
				if (sqrt(d.dot(d)) > 10 ) {
					imshow("hand", hand);
//					if (cv::waitKey(0) != 'n') {
//						return;
//					}
//					cv::waitKey(1);

					extend(hand, img, (currPoint), meanColor, depth);
				} else {
					continue;
				}
				break;
			}
		}
	}
}


}// Namespace iez
