#pragma once
#include "ImageSource.h"
#include <opencv2/opencv.hpp>

#include <OpenNI.h>

#include <QtCore/qthread.h>
#include <QtCore/qtimer.h>
#include <qreadwritelock.h>
#include <QKeyEvent>

namespace iez {
class ImageSourceOpenNI : public ImageSourceBase
{
	Q_OBJECT
public:
	ImageSourceOpenNI(int fps = 30);
	~ImageSourceOpenNI(void);

	int init(const char* deviceURI = openni::ANY_DEVICE);
	bool isInitialized();


	cv::Mat getDepthMat() const;
	cv::Mat getColorMat() const;

	openni::VideoStream& getColorStream();
	openni::VideoStream& getDepthStream();
private:
//	void run(); // Overriden QThread run


	int deviceInit(const char* deviceURI);
	int streamInit(void);

	void readDepth();
	void readColor();
	int m_width, m_height;

	cv::Mat m_depthMat;
	cv::Mat m_colorMat;
	openni::Device device;
	openni::VideoStream m_depthStream, m_colorStream;
	openni::VideoStream *m_streams[2];
	openni::VideoFrameRef m_depthFrame, m_colorFrame;

	QTimer m_timer;
	mutable QReadWriteLock depth_rwlock;
	mutable QReadWriteLock color_rwlock;

	int m_fps;
	bool m_initialized;
	QThread m_thread;
	int m_failCount;
public slots:
	void keyEvent(QKeyEvent *event);
private slots:
	void update();
};


}
