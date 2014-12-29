
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
//#include <QApplication>
// Custom
#include "ImageSource.h"
#include "ImageSourceFreenect.h"
#include "Processing.h"
#include "WindowManager.h"
#include "main.h"

using namespace cv;

//#include "ColorSegmentation.h"


#include "ImageDescriptor.h"
void test()
{
//	QThread t;
	iez::ColorSegmentation seg;
	seg.buildDatabaseFromFiles("../database/colorDB_files.txt");
//	const std::list<QPolygon> polygons = seg.polygonsFromFile("x.bmp");


	iez::ImageSourceFreenect kinect(0);
	kinect.streamInit(FREENECT_RESOLUTION_MEDIUM);
	iez::ImageDescriptor imageDescriptor(&kinect);
	imageDescriptor.refresh();

	QApplication::exec();
}
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
//	test();
//	return 0;
	iez::ImageSourceFreenect kinect(0);
	kinect.streamInit(FREENECT_RESOLUTION_MEDIUM);

	iez::ImageDescriptor imageDescriptor(&kinect);
	iez::ColorSegmentation seg;
//	seg.buildDatabaseFromFiles("../database/colorDB_files.txt");

	iez::Processing *processing = new iez::Processing(kinect);
	processing->init();

	return QApplication::exec();
}
