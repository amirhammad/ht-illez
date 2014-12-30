
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
#include "ColorSegmentation.h"
#include "ImageDescriptor.h"
#include "Processing.h"
#include "WindowManager.h"
#include "main.h"

using namespace cv;



int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// Camera init
	iez::ImageSourceFreenect kinect(0);
	kinect.streamInit(FREENECT_RESOLUTION_MEDIUM);

	/**
	 * Processing
	 */

	// TODO: can edit files
	iez::ImageDescriptor *imageDescriptor = new iez::ImageDescriptor(&kinect);

	iez::ColorSegmentation::buildDatabaseFromFiles("../database/colorDB_files.txt");

	iez::Processing *processing = new iez::Processing(kinect);

	return QApplication::exec();
}
