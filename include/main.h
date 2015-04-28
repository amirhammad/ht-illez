#pragma once

#include "ImageSourceFreenect.h"
#include <opencv2/opencv.hpp>
#include "qmutex.h"
namespace iez {
class ImageSourceArtificial;
extern ImageSourceArtificial *imageSourceArtificial;

class ImageSourceArtificial : public ImageSourceBase {
	Q_OBJECT
public:
	ImageSourceArtificial();
	virtual ~ImageSourceArtificial();
	cv::Mat getColorMat() const;
	cv::Mat getDepthMat() const;
	void setColorMat(const cv::Mat &src);
	void setDepthMat(const cv::Mat &src);
public slots:
	void pause(bool p = true);
private:
	cv::Mat m_color;
	cv::Mat m_depth;
	mutable QMutex m_mutex;
};

}
