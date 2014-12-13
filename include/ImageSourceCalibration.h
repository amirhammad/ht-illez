#pragma once
#include <stdint.h>
#include <opencv2/opencv.hpp>

namespace iez {
class CImageSourceCalibration
{
public:
	CImageSourceCalibration(void);
	~CImageSourceCalibration(void);
	void calibratePixel(const cv::Mat depth, int y, int x, int *rgb_x, int *rgb_y);
	void calibrate(cv::Mat &calibratedImage ,const cv::Mat &depth, const cv::Mat &image);
};


}