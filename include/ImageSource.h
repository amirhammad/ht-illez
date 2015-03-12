#pragma once
#include <opencv2/opencv.hpp>
#include <qobject.h>
namespace iez {

class ImageSourceBase : public QObject {
	Q_OBJECT
public:
	ImageSourceBase()
	:	m_sequence(-1) {

	}
	virtual cv::Mat getColorMat() const = 0;
	virtual cv::Mat getDepthMat() const = 0;

	virtual ~ImageSourceBase(){};
	int getSequence() const {
		return m_sequence;
	}

protected:
	int m_sequence;
signals:
	void frameReceived();
};

}
