
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

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	new iez::MainWindow();

	return QApplication::exec();
}
