#pragma once
#include "ImageSource.h"
#include <opencv2/opencv.hpp>

#include <OpenNI.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>

namespace iez {
class ImageSourceOpenNI : private QThread, public ImageSourceBase
{
	Q_OBJECT
public:
	ImageSourceOpenNI(int fps = 30);
	~ImageSourceOpenNI(void);

	bool isInitialized();
	void update(void);

	cv::Mat getDepthMat();
	cv::Mat getColorMat();

private:
	void run(); // Overriden QThread run

	int init(void);
	int deviceInit(void);
	int streamInit(void);

	int m_width, m_height;

	cv::Mat m_depthMat;
	cv::Mat m_colorMat;
	openni::Device device;
	openni::VideoStream m_depthStream, m_colorStream;
	openni::VideoStream *m_streams[2];
	openni::VideoFrameRef m_depthFrame, m_colorFrame;

	QMutex depth_mutex;
	QMutex color_mutex;

	const int m_fps;
	bool m_initialized;
};


}
