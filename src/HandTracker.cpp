#include "HandTracker.h"
#include <assert.h>
#include <vector>
//#include <eigen3/Eigen/Eigen>
//#include <armadillo>
#define MSG(x) std::cout<<x<<std::endl

using namespace cv;
//using namespace std;
namespace iez {

void HandTracker::findHandFromCenter(const cv::Mat& bgr, const cv::Mat& depth)
{
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	int rows = bgr.rows;
	int cols = bgr.cols;

	Point2i center(cols/2,rows/2);
	Mat hand;
	Mat img;
//	img.create(bgr.rows, bgr.cols, CV_8UC3);
//	medianBlur(bgr,img,3);
//	m_window.imShow("img", img);
	bgr.copyTo(img);

	cvtColor(bgr, hand, COLOR_BGR2GRAY);
//	cvtColor(img, img, COLOR_BGR2YCrCb);
//	bgr.copyTo(hand);
//	Point3f meanColor = img.at<Point3_<uint8_t> >(center.y, center.x);
//	std::cout<<"COLOR: "<<meanColor.x<<" "<<meanColor.y<<" "<<meanColor.z<<std::endl;

//	Point3f reference(0.36f,0.42f,93);
//	qDebug("%5.3f %5.3f %5.3f\n", meanColor.x, meanColor.y, meanColor.z);
	for (int i = 0; i < bgr.rows; ++i) {
		for (int j = 0; j < bgr.cols; ++j) {
//			const Point3f pt = bgr.at<Point3_<uint8_t> >(i,j);
//			const float diff = pt.z - reference.z;
//			if (sqrt(diff*diff) < 10) {
//				hand.at<uint8_t>(i, j) = 0;
//			} else {
//				hand.at<uint8_t>(i, j) = 255;
//			}
			double probability = 0;//segmentation.getProbability(bgr.at<Point3_<uint8_t> >(i,j));
			if (probability > 0.1) {
				img.at<Point3_<uint8_t> > (i,j).x = 0;
				img.at<Point3_<uint8_t> > (i,j).y = 0;
				img.at<Point3_<uint8_t> > (i,j).z = 0;
			} else {
//				qDebug("%f", probability);
			}
		}
	}

//	ImageStatistics x(img);
	Mat all;
//	qDebug("MAX ALL: %d", seg.maxAll);
	all.create(256,256,CV_8UC1);
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j<256; j++) {
//				all.at<uint8_t>(i,j) = std::min<int>(255, seg.m_CrCbCountAll[i][j]/10);
//				skin.at<uint8_t>(i,j) = std::min<int>(255, seg.m_CrCbCountSkin[i][j]);
		}
	}
//		window.imShow("ColorSegmentation: all", all);
	WindowManager::getInstance().imShow("Pure img", img);
	WindowManager::getInstance().imShow("ALL histogram", all);
//	extend(hand, img, center, Point3f(0.34f,0.42f,0.24), 4);
//	m_window.imShow("hand", hand);
}

cv::Point3f HandTracker::interpolateToColor(const cv::Mat &bgr, const cv::Point2f &coord)
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

float HandTracker::calculateGradientInDirection(
		const cv::Point3f &prevColor,
		const cv::Point3f &nextColor)
{
	Point3f diffColor(static_cast<int>(nextColor.x) - static_cast<int>(prevColor.x),
					static_cast<int>(nextColor.y) - static_cast<int>(prevColor.y),
					static_cast<int>(nextColor.z) - static_cast<int>(prevColor.z));

//	return sqrt(diffColor.dot(diffColor));
	return sqrt(diffColor.x*diffColor.x + diffColor.y*diffColor.y + diffColor.z*diffColor.z);
}

void HandTracker::extend(cv::Mat &hand, const cv::Mat& img, cv::Point2f center,
		cv::Point3f meanColor, int depth)
{
	depth--;
	if (depth == 0) {
//		m_window.imShow("hand", hand);
		return;
	}
	QVector<double> x,y;

//	CColorRedGreen meanRedGreenColor(meanColor);

	for (float angle = 0; angle < 2*M_PI; angle += M_PI/12) {
		const float step_c = 1;//dont change
		const float maxHandSize_c = 140; //in pixels
		float dx = std::cos(angle);
		float dy = std::sin(angle);


		// center, img
		float step;
		int i;
		Point3f mc(meanColor);
		for (step = step_c,i=1; step<maxHandSize_c; step+=step_c) {

			Point2f currPoint(center.x+dx*step, center.y+dy*step);
			if (currPoint.x > img.cols
			|| currPoint.y > img.rows
			|| currPoint.x < 0
			|| currPoint.y < 0 ) {
				break;
			}
			const Point2f prevPoint(center.x+dx*(step-1), center.y+dy*(step-1));
			const Point2f nextPoint(center.x+dx*(step+1), center.y+dy*(step+1));
			// calculate gradient

			// if current gradient in that direction is bigger than threshold, save border point
			const Point3f currColor = interpolateToColor(img, currPoint);
			const Point3f prevColor = interpolateToColor(img, prevPoint);
//			const Point3f nextColor = interpolateToColor(img, nextPoint);

//			const float gradient = calculateGradientInDirection(prevColor, nextColor);
			const float gradient = calculateGradientInDirection(meanColor, currColor);

			hand.at<uint8_t>(currPoint.y, currPoint.x) = (1.0f-gradient)*255;//static_cast<uint8_t>(std::min<float>(255,gradient))*0 + 0;


			if (depth == -2) {
				if (angle == 0) {
					x.append(currPoint.x);
					y.append(gradient);
				}
			} else
			if (gradient > 0.05) {
				Point2f d(currPoint-center);
				if (sqrt(d.dot(d)) > 5 ) {

//					m_window.imShow("hand", hand);
//					if (cv::waitKey(0) != 'n') {
//						return;
//					}
//					cv::waitKey(1);
//					QThread::pause();
					if (depth>0) {
						extend(hand, img, currPoint, meanColor, depth);
					}
				} else {
					continue;
				}
				break;
			}
		}
	}

	if (depth == -2) {
		WindowManager::getInstance().plot("X1", x, y);
	}
}




HandTracker::HandTracker()
{
//	const char * dbPath = "/home/amir/git/amirhammad/diplomovka/Skin_NonSkin.txt";
//	if (segmentation.buildDatabaseFromRGBS(dbPath)) {
//		qDebug("database built");
//		Mat all,skin;
//		all.create(256, 256, CV_8UC1);
//		skin.create(256, 256, CV_8UC1);
//		for (int i = 0; i < 256; i++) {
//			for (int j = 0; j<256; j++) {
////				all.at<uint8_t>(i,j) = std::min<int>(255, segmentation.m_CrCbCountAll[i][j]);
////				skin.at<uint8_t>(i,j) = std::min<int>(255, segmentation.m_CrCbCountSkin[i][j]);
//			}
//		}
//		WindowManager::getInstance().imShow("ColorSegmentation: all", all);
//		WindowManager::getInstance().imShow("ColorSegmentation: skin", skin);
//	} else {
//		qDebug("failed to build database");
//	}
}

} // Namespace iez
