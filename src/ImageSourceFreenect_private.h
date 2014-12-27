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
		int init(void);

		bool isInitialized();
		void update(void);

		cv::Mat getDepthMat();
		cv::Mat getColorMat();

		int sequence() { return m_sequence; }
		int streamInit(freenect_resolution resolution);
	protected:

		int deviceInit(void);


		// Do not call directly even in child
		void VideoCallback(void *video, uint32_t timestamp)
		{
			color_mutex.lock();
			uint8_t *d = static_cast<uint8_t*>(video);
	//		memcpy(m_colorMat.data, video, m_height*m_width*3);
			cv::cvtColor(cv::Mat(m_height, m_width, CV_8UC3, video), m_colorMat, cv::COLOR_RGB2BGR);

			color_mutex.unlock();
		}
		// Do not call directly even in child
		void DepthCallback(void *depth, uint32_t timestamp)
		{
			return;
			uint16_t *d = static_cast<uint16_t *>(depth);
			qDebug("depth %d %d %d", d[0], d[1], d[2]);
			for (int i=0; i<640*480; i++) {
				int pval = m_t_gamma[d[i]];
				int lb = pval & 0xff;
				switch (pval>>8) {
					case 0:
						m_depthMat.data[3*i+0] = 255;
						m_depthMat.data[3*i+1] = 255-lb;
						m_depthMat.data[3*i+2] = 255-lb;
						break;
					case 1:
						m_depthMat.data[3*i+0] = 255;
						m_depthMat.data[3*i+1] = lb;
						m_depthMat.data[3*i+2] = 0;
						break;
					case 2:
						m_depthMat.data[3*i+0] = 255-lb;
						m_depthMat.data[3*i+1] = 255;
						m_depthMat.data[3*i+2] = 0;
						break;
					case 3:
						m_depthMat.data[3*i+0] = 0;
						m_depthMat.data[3*i+1] = 255;
						m_depthMat.data[3*i+2] = lb;
						break;
					case 4:
						m_depthMat.data[3*i+0] = 0;
						m_depthMat.data[3*i+1] = 255-lb;
						m_depthMat.data[3*i+2] = 255;
						break;
					case 5:
						m_depthMat.data[3*i+0] = 0;
						m_depthMat.data[3*i+1] = 0;
						m_depthMat.data[3*i+2] = 255-lb;
						break;
					default:
						m_depthMat.data[3*i+0] = 0;
						m_depthMat.data[3*i+1] = 0;
						m_depthMat.data[3*i+2] = 0;
						break;
				}
			}


		}

		int m_width, m_height;

		cv::Mat m_depthMat;
		cv::Mat m_colorMat;

		QMutex depth_mutex;
		QMutex color_mutex;

		int m_sequence;
		const int m_fps;
		bool m_initialized;

		uint16_t m_t_gamma[2048];
	};

}


