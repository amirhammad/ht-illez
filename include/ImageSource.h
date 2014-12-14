#pragma once
#include <mutex>
#include <opencv2/opencv.hpp>
#include "ImageSourceCalibration.h"
#include <OpenNI.h>


namespace iez {
class CImageSource
{
public:
	CImageSource(int fps = 30);
	~CImageSource(void);
	int init(void);

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

	CImageSourceCalibration calibration;
private:
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

	std::mutex depth_mutex;
	std::mutex color_mutex;

	const int m_fps;
};


}
