#include "Processing.h"
#include <stdint.h>
#include <assert.h>
#include <opencv2/imgproc.hpp>

using namespace iez;
using namespace cv;

CProcessing::CProcessing(void) 
{

}


CProcessing::~CProcessing(void)
{
	
}


void CProcessing::process(const Mat &bgr, const Mat &depth)
{
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	Mat gray;
	Mat color;

	bgr.copyTo(color);

	gray.create(bgr.rows, bgr.cols, CV_8UC1);

	filterDepth(color, depth, 500, 2000);

	imshow("Original:", bgr);
	imshow("filtered", color);
}

void iez::CProcessing::findHandFromCenter(const cv::Mat& bgr, const cv::Mat& depth)
{
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	handTracker.findHandFromCenter(bgr, depth);
}

void CProcessing::filterDepth(Mat &dst, const Mat &depth, int near, int far)
{
	assert(dst.rows == depth.rows);
	assert(dst.cols == depth.cols);

	for (int y = 0; y < dst.rows; ++y) {
		for (int x = 0; x < dst.cols; ++x) {

			int val = depth.at<uint16_t>(y,x);

			if (val > near && val < far) {
				// keep color
			} else {
				dst.at<Point3_<uint8_t> >(y,x) = Point3_<uint8_t>(0,0,0);

			}
		}
	}
}
