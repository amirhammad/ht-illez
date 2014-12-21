#pragma once
#include <opencv2/opencv.hpp>

#include <OpenNI.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>

namespace iez {
class CImageSource:private QThread
{
	Q_OBJECT
public:
	CImageSource(int fps = 30);
	~CImageSource(void);
	int init(void);

	bool isInitialized();
	void update(void);

	cv::Mat getDepthMat()
	{
//		cv::Mat mat;
//		mat.create(m_height, m_width, CV_16UC1);
		depth_mutex.lock();
		if (m_depthFrame.isValid()) {
			memcpy(m_depthMat.data, m_depthFrame.getData(), m_depthFrame.getDataSize());
		}
		depth_mutex.unlock();
		return m_depthMat;
	}

	cv::Mat getColorMat()
	{
//		cv::Mat mat;
//		mat.create(m_height, m_width, CV_8UC3);
		color_mutex.lock();
		if (m_colorFrame.isValid()) {
			memcpy(m_colorMat.data, m_colorFrame.getData(), m_colorFrame.getDataSize());
		}
		color_mutex.unlock();
		return m_colorMat;
	}

	int sequence() { return m_sequence; }

private:
	void run() {
		while(1) {
			update();
			yieldCurrentThread();
			msleep(1);
		}
	}
	int deviceInit(void);
	int streamInit(void);

	int m_width, m_height;
	cv::Mat m_depthMat;
	cv::Mat m_colorMat;
	cv::Mat calibratedImage;
	openni::Device device;
	openni::VideoStream m_depthStream, m_colorStream;
	openni::VideoStream *m_streams[2];
	openni::VideoFrameRef m_depthFrame, m_colorFrame;

	QMutex depth_mutex;
	QMutex color_mutex;

	int m_sequence;
	const int m_fps;
	bool m_initialized;
};


}
