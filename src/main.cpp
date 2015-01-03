
// opencv
#include <opencv2/opencv.hpp>

// STL
#include <iostream>
#include <string>

// armadillo
//#include <armadillo>

// libfreenect
#include <libfreenect/libfreenect.hpp>

// Qt
//#include <QThread>
#include <QApplication>
// Custom
#include "ImageSource.h"
#include "ImageSourceFreenect.h"
#include "ImageSourceOpenNI.h"
#include "ColorSegmentation.h"
#include "ImageDescriptor.h"
#include "Processing.h"
#include "WindowManager.h"
#include "main.h"

using namespace cv;


iez::ImageSourceArtificial *iez::imageSourceArtificial;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// Camera init
//	iez::ImageSourceFreenect *kinectFreenect = new iez::ImageSourceFreenect(0);
	iez::ImageSourceBase *kinect = new iez::ImageSourceFreenect();

	/**
	 * Processing
	 */

	iez::imageSourceArtificial = new iez::ImageSourceArtificial();
	// TODO: can edit files
	iez::ImageDescriptor *imageDescriptor = new iez::ImageDescriptor(iez::imageSourceArtificial);

	iez::ColorSegmentation::buildDatabaseFromFiles("../database/colorDB_files.txt");

	iez::Processing *processing = new iez::Processing(kinect);

	return QApplication::exec();
}

cv::Mat iez::ImageSourceArtificial::getColorMat()
{
	QMutexLocker locker(&m_mutex);
	cv::Mat ret;
	m_color.copyTo(ret);
	return ret;
}

iez::ImageSourceArtificial::ImageSourceArtificial()
{
	QMutexLocker locker(&m_mutex);
	m_color.create(480, 640, CV_8UC3);
}

void iez::ImageSourceArtificial::setColorMat(const cv::Mat& src)
{
	QMutexLocker locker(&m_mutex);
	cvtColor(src, m_color, cv::COLOR_RGB2BGR);
	m_sequence++;
}
