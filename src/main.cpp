#include <opencv2/opencv.hpp>
#include <time.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include "ImageSource.h"
#include "Processing.h"
using namespace cv;
iez::CImageSource g_kinect;
iez::CProcessing g_processing;


static int params[20]={11, 139, 0, 255, 95, 169, 174, 118};
static int trackbar_h = 8;
static int trackbar_s = 14;
static int trackbar_v = 15;
static int trackbar_h_range = 26;
static int trackbar_s_range = 28;
static int trackbar_v_range = 6;
void processHSVFilter(const cv::Mat &orig)
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


	orig.copyTo(color);
	cvtColor(orig,image,COLOR_RGB2YCrCb);
	cvtColor(orig,gray,COLOR_BGR2GRAY);


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
	medianBlur(mask*255,mask,5);
	imshow("median",mask);
	imshow("main",orig);
	Canny(mask,canny,params[6],params[7]);
//		imshow("black",color);
	imshow("CANNY", canny);
}

void kinect_update(std::string msg)
{
	while(1) {
		g_kinect.update();
		usleep(15000);
	}
}

void process1()
{
	using namespace std;
	g_kinect.init();
	int i;
//	for (i=0;i<6;i++)
//	{
//		std::stringstream stream;
//		stream<<"P";
//		stream<<i;
//		std::string s = stream.str();
//		createTrackbar(s,"main", &params[i], 255);
//	}
//	for(i=6;i<8;i++)
//	{
//		std::stringstream stream;
//		stream<<"P";
//		stream<<i;
//		std::string s = stream.str();
//		createTrackbar(s,"CANNY", &params[i], 255);
//	}
	std::thread kinectThread(kinect_update, "ahoj");
	while (1)
	{
		Mat rgb = g_kinect.getColorMat();
		Mat color;
		cvtColor(rgb, color, COLOR_RGB2BGR);
		const Mat &depth = g_kinect.getDepthMat();

		g_processing.process(color, depth);
		processHSVFilter(color);


		int k = waitKey(1) ;
		switch (k) {
		case 's':
		{
			string xd = string("image.bmp");
			//imwrite(xd.data(), X);
			//imwrite("depth.bmp",g_kinect.getDepthMat());
			cout<<"writing image"<<xd<<endl;
		}
			break;

		case 'q':
			return;
		}

	}
}

int main() 
{
	process1();
	system("pause");
	return 0;
}
