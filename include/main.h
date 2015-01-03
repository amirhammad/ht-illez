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
	cv::Mat getColorMat();
	cv::Mat getDepthMat() { return cv::Mat(640, 480, CV_16UC1); }
	void setColorMat(const cv::Mat &src);
	int getSequence() const;

private:
	cv::Mat m_color;
	QMutex m_mutex;
	int m_sequence;
};


}
