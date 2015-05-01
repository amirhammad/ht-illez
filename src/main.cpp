
// Custom
#include "main.h"
#include "ImageSourceOpenNI.h"
#include "MainWindow.h"


// opencv
#include <opencv2/opencv.hpp>

// STL
#include <iostream>
#include <string>

// libfreenect
#include <libfreenect/libfreenect.hpp>

// Qt
//#include <QThread>
#include <QApplication>
#include <QKeyEvent>


iez::ImageSourceArtificial *iez::imageSourceArtificial;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	iez::ImageSourceArtificial artif;
	iez::imageSourceArtificial = &artif;
	new iez::MainWindow();

	return QApplication::exec();
}

cv::Mat iez::ImageSourceArtificial::getColorMat() const
{
	QMutexLocker locker(&m_mutex);
	cv::Mat ret;
	m_color.copyTo(ret);
	return ret;
}

cv::Mat iez::ImageSourceArtificial::getDepthMat() const
{
	QMutexLocker locker(&m_mutex);
	return m_depth;
}

iez::ImageSourceArtificial::ImageSourceArtificial()
:	ImageSourceBase()
{
	QMutexLocker locker(&m_mutex);
	m_color.create(480, 640, CV_8UC3);
}

iez::ImageSourceArtificial::~ImageSourceArtificial()
{
}

void iez::ImageSourceArtificial::setColorMat(const cv::Mat& src)
{
	QMutexLocker locker(&m_mutex);
	cvtColor(src, m_color, cv::COLOR_RGB2BGR);
	m_sequence++;
}

void iez::ImageSourceArtificial::setDepthMat(const cv::Mat &src)
{
	QMutexLocker locker(&m_mutex);
	m_depth = src;
}

void iez::ImageSourceArtificial::pause(bool p)
{
	Q_UNUSED(p);
}
