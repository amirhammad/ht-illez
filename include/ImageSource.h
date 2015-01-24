#pragma once
#include <opencv2/opencv.hpp>

namespace iez {

class ImageSourceBase {
public:
	ImageSourceBase()
	:	m_sequence(0) {

	}
	virtual cv::Mat getColorMat() const = 0;
	virtual cv::Mat getDepthMat() const = 0;

	virtual ~ImageSourceBase(){};
	int getSequence() const {
		return m_sequence;
	}

protected:
	int m_sequence;
};

}
