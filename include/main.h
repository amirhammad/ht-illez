#pragma once

#include "ImageSourceFreenect.h"
#include <opencv2/opencv.hpp>
#include "qmutex.h"
namespace iez {
class ImageSourceArtificial;
extern ImageSourceArtificial *imageSourceArtificial;

class ImageSourceArtificial:public ImageSourceBase {
public:
	ImageSourceArtificial();
	cv::Mat getColorMat() const;
	cv::Mat getDepthMat() const { return cv::Mat(640, 480, CV_16UC1); }
	void setColorMat(const cv::Mat &src);

private:
	cv::Mat m_color;
	mutable QMutex m_mutex;
};

class Fps {
public:
	Fps();
	void tick();
	float fps() const;

private:
	clock_t m_timeLast;

};

}
