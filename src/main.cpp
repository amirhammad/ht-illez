
// opencv
#include <opencv2/opencv.hpp>

#include <time.h>

// STL
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

// armadillo
#include <armadillo>

// Qt
//#include <QThread>
//#include <QApplication>
#include <QtCore>
#include <QtGui>
#include <thread>
// Custom
#include "ImageSource.h"
#include "Processing.h"
#include "main.h"
using namespace cv;


//void qplottest()
//{
//	QMainWindow *w = new QMainWindow();
//	w->setFixedSize(QSize(640, 480));
//	QWidget *l = new QWidget();
//	w->setCentralWidget(l);
//
//	QCustomPlot *customPlot = new QCustomPlot(l);
////	customPlot->setViewport(QRect(QPoint(0,0), QSize(640,480)));
////	customPlot->setBaseSize(QSize(640,480);
//	customPlot->setFixedSize(640,480);
//	// generate some data:
//	QVector<double> x(101), y(101); // initialize with entries 0..100
//	for (int i=0; i<101; ++i)
//	{
//	  x[i] = i/50.0 - 1; // x goes from -1 to 1
//	  y[i] = x[i]*x[i]*std::cos(x[i]); // let's plot a quadratic function
//	}
//	// create graph and assign data to it:
//	customPlot->addGraph();
//	customPlot->graph(0)->setData(x, y);
//	// give the axes some labels:
//	customPlot->xAxis->setLabel("x");
//	customPlot->yAxis->setLabel("y");
//	// set axes ranges, so we see all data:
//	customPlot->xAxis->setRange(-1, 1);
//	customPlot->yAxis->setRange(0, 1);
//	customPlot->replot();
//	w->show();
//}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

//	qplottest();
//	iez::CMainWindow mainWindow;
//	mainWindow.init();
	iez::CImageSource kinect(30);
	iez::CProcessing processing(kinect);
	kinect.init();
//	while(kinect.getColorMat().data[30] == 0);
	processing.init();

	return QApplication::exec();
}
