
// opencv
#include <opencv2/opencv.hpp>

// STL
#include <iostream>
#include <string>

// armadillo
//#include <armadillo>

// Qt
//#include <QThread>
//#include <QApplication>
// Custom
#include "ImageSource.h"
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
	seg.buildDatabaseFromFiles("colorDB_files.txt");
//	const std::list<QPolygon> polygons = seg.polygonsFromFile("x.bmp");

//	iez::ImageDescriptor imageDescriptor;
//	imageDescriptor.refresh();
//	imageDescriptor.moveToThread(&t);
	QApplication::exec();
}
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	test();
	return 0;
	iez::CImageSource kinect(30);
	kinect.init();
	if (!kinect.isInitialized()) {
		return -1;
	}
	iez::CProcessing processing(kinect);
	processing.init();

	return QApplication::exec();
}
