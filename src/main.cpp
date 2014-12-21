
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

#include "ColorSegmentation.h"

void test()
{

}
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	test();

	iez::CImageSource kinect(30);
	kinect.init();
	if (!kinect.isInitialized()) {
		return -1;
	}
	iez::CProcessing processing(kinect);
	processing.init();

	return QApplication::exec();
}
