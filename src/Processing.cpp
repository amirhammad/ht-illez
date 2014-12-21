#include "Processing.h"
#include "ImageSource.h"
#include <stdint.h>
#include <assert.h>
#include <QtGui>
#include <QtCore>
//#include <QImage>
#include <opencv2/imgproc.hpp>


//using namespace cv;
namespace iez {

CProcessing::CProcessing(CImageSource &imgsrc)
: imageSource(imgsrc)
, window()
, handTracker(window)
, calculateHandTracker(false)
{
	connect(&window, SIGNAL(keyPressed(QKeyEvent *)), this, SLOT(keyPressEvent(QKeyEvent *)));
	connect(&window, SIGNAL(closed()), this, SLOT(closeEvent()));
}


CProcessing::~CProcessing(void)
{
	terminate();
}


void CProcessing::process(const cv::Mat &bgr, const cv::Mat &depth)
{
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	cv::Mat gray;
	cv::Mat color;

	bgr.copyTo(color);
	cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
//	gray.create(bgr.rows, bgr.cols, CV_8UC1);

	filterDepth(color, depth, 500, 2000);

	window.imShow("Original", bgr);
//	window.imShow("color/depth", color);
//	waitKey(1);
//	window.imShow("filtered", gray);
	/*
	QVector<double> x(101), y(101); // initialize with entries 0..100
	for (int i=0; i<101; ++i)
	{
	  x[i] = i/50.0 - 1; // x goes from -1 to 1
	  y[i] = x[i]*x[i]*std::cos(x[i]); // let's plot a quadratic function
	}

	window.plot("hello", x,y);
	*/
	handTracker.findHandFromCenter(bgr, depth);
}

void CProcessing::filterDepth(cv::Mat &dst, const cv::Mat &depth, int near, int far)
{
	assert(dst.rows == depth.rows);
	assert(dst.cols == depth.cols);

	for (int y = 0; y < dst.rows; ++y) {
		for (int x = 0; x < dst.cols; ++x) {

			int val = depth.at<uint16_t>(y,x);

			if (val > near && val < far) {
				// keep color
			} else {
				dst.at<cv::Point3_<uint8_t> >(y,x) = cv::Point3_<uint8_t>(0,0,0);

			}
		}
	}
}

int CProcessing::init()
{
	start();
	setPriority(IdlePriority);
}

void iez::CProcessing::run()
{
	int sequence = 0;
	while (1)
	{
		int seq = imageSource.sequence();
		if (sequence == seq) {
			yieldCurrentThread();
			continue;
		}
		sequence = seq;

		cv::Mat rgb = imageSource.getColorMat();
		cv::Mat color;
		cv::cvtColor(rgb, color,cv::COLOR_RGB2BGR);
		cv::Mat depth = imageSource.getDepthMat();

		process(color, depth);
		processHSVFilter(color);
		if (calculateHandTracker) {
			handTracker.findHandFromCenter(color,depth);
			calculateHandTracker = false;
		}
	}
}
static int params[20]={11, 139, 0, 255, 95, 169, 174, 118};
void CProcessing::processHSVFilter(const cv::Mat &orig)
{
	using namespace cv;
	Mat color,gray,image,canny;

	Mat mask(orig.rows, orig.cols, CV_8UC1);
	for (int i=0;i<6;i++)
	{
		std::stringstream stream;
		stream<<"P";
		stream<<i;
		std::string s = stream.str();
		createTrackbar(s,"main", &params[i], 255);
	}
	for(int i=6;i<8;i++)
	{
		std::stringstream stream;
		stream<<"P";
		stream<<i;
		std::string s = stream.str();
		createTrackbar(s,"CANNY", &params[i], 255);
	}



	cvtColor(orig,color,COLOR_RGB2HSV);
	cvtColor(orig,gray,COLOR_BGR2GRAY);
	GaussianBlur(orig,color,Size(5,5),0);
	cvtColor(color,image,COLOR_RGB2HSV);
	for(int i=0;i<image.rows;i++)
	{
		for(int j=0;j<image.cols;j++)
		{

			//cout<<image.cols;
			//image.at<uchar>(Point(i,j)) = 0;
			//std::cout<<image.at<uint8_t>(Point(i,j))<<std::endl;
			Point3_<uint8_t> hsv;
			hsv = image.at<Point3_<uint8_t> >(i,j);
			if(
				(
					params[0]	<= params[1]
				&&	hsv.x		>= params[0]
				&&	hsv.x		<= params[1]
				)
			||
				(
					params[0]	> params[1]
				&&	(hsv.x		>= params[0]
				||	hsv.x		<= params[1])

				)
			)
			{
				if(
					(
						params[2]	<= params[3]
					&&	hsv.y		>= params[2]
					&&	hsv.y		<= params[3]
					)
				||
					(
						params[2]	> params[3]
					&&	(hsv.y		>= params[2]
					||	hsv.y		<= params[3])

					)
				)
				{
					if(
						(
							params[4]	<= params[5]
						&&	hsv.z		>= params[4]
						&&	hsv.z		<= params[5]
						)
					||
						(
							params[4]	> params[5]
						&&	(hsv.z		>= params[4]
						||	hsv.z		<= params[5])

						)
					)
					{
						mask.at<uint8_t>(i,j)=0;
					}
					else
						mask.at<uint8_t>(i,j)=1;
				}
				else
					mask.at<uint8_t>(i,j)=1;
			}
			else
				mask.at<uint8_t>(i,j)=1;

			//gray.at<uint8_t>(i,j) = mask.at<uint8_t>(i,j)*gray.at<uint8_t>(i,j);
			color.at<uint8_t>(i,j*3)=mask.at<uint8_t>(i,j)*color.at<uint8_t>(i,j*3);
			color.at<uint8_t>(i,j*3+1)=mask.at<uint8_t>(i,j)*color.at<uint8_t>(i,j*3+1);
			color.at<uint8_t>(i,j*3+2)=mask.at<uint8_t>(i,j)*color.at<uint8_t>(i,j*3+2);
		}//CYCLE
	}
	medianBlur(mask*255,mask,9);
//	imshow("median",mask);
//	imshow("main",color);
	Canny(mask,canny,params[6],params[7]);
//		imshow("black",color);
//	imshow("CANNY", canny);
}

void CProcessing::keyPressEvent(QKeyEvent* keyEvent)
{
	switch (keyEvent->key()) {
	case Qt::Key_Escape:
		::exit(0);
		break;
	case Qt::Key_F:
		calculateHandTracker = true;
		break;
	}
}

void CProcessing::closeEvent()
{
	::exit(0);
}

}
