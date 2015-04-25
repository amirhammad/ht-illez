#pragma once
#include <opencv2/opencv.hpp>
#include <qobject.h>

namespace iez {

class ImageSourceBase : public QObject {
	Q_OBJECT
public:
	ImageSourceBase(QObject *parent = 0)
	:	m_sequence(-1) {

	}

	virtual cv::Mat getColorMat() const = 0;
	virtual cv::Mat getDepthMat() const = 0;

	virtual ~ImageSourceBase(){}

	int getSequence() const {
		return m_sequence;
	}

protected:
	int m_sequence;

public slots:
	/**
	 *
	 */
	virtual void pause(bool p = true) = 0;
signals:
	void frameReceived();
};

}
