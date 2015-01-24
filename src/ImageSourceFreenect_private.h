#pragma once
#include <opencv2/opencv.hpp>

#include <QtCore/qmutex.h>
#include <libfreenect/libfreenect.hpp>

namespace iez_private {

class ImageSourceFreenectDevice_private:public Freenect::FreenectDevice
	{
	public:
		ImageSourceFreenectDevice_private(freenect_context *_ctx, int _index);
		~ImageSourceFreenectDevice_private(void);
//		int init(void);

		bool isInitialized();

		cv::Mat getDepthMat() const;
		cv::Mat getColorMat() const;

		int getSequence() { return m_sequence; }
		int streamInit(freenect_resolution resolution);
	protected:

		int deviceInit(void);


		// Do not call directly even in child
		void VideoCallback(void *video, uint32_t timestamp)
		{
			QMutexLocker lock(&color_mutex);
			m_sequence++;
			memcpy(m_colorMat.data, video, m_height*m_width*3);
		}
		// Do not call directly even in child
		void DepthCallback(void *depth, uint32_t timestamp)
		{
			QMutexLocker lock(&depth_mutex);
			memcpy(m_depthMat.data, depth, m_width*m_height*2);
		}

		int m_width, m_height;

		cv::Mat m_depthMat;
		cv::Mat m_colorMat;

		mutable QMutex depth_mutex;
		mutable QMutex color_mutex;

		int m_sequence;
		const int m_fps;
		bool m_initialized;

//		uint16_t m_t_gamma[2048];
	};

}


