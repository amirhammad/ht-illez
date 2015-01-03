#include "Processing.h"
#include "ImageSource.h"
#include <stdint.h>
#include <assert.h>
#include <QtGui>
#include <QtCore>
//#include <QImage>
#include <opencv2/imgproc.hpp>
#include "main.h"

//using namespace cv;
namespace iez {

Processing::Processing(ImageSourceBase *imgsrc)
: 	m_imageSource(imgsrc)
, 	m_calculateHandTracker(false)
,	m_statsList(0)
{
//	connect(&m_window, SIGNAL(keyPressed(QKeyEvent *)), this, SLOT(keyPressEvent(QKeyEvent *)));
//	connect(&m_window, SIGNAL(closed()), this, SLOT(closeEvent()));
	m_segmentation = new ColorSegmentation();
	start();
}


Processing::~Processing(void)
{
	delete m_segmentation;
}


void Processing::process(const cv::Mat &bgr, const cv::Mat &depth)
{
//	qDebug("%d %d %d %d ", bgr.rows, depth.rows, bgr.cols, depth.cols);
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	WindowManager::getInstance().imShow("Original", bgr);
	cv::Mat bgrRoi;
	cv::Mat bgrDepthFiltered;
	processDepthFiltering(bgr, depth, bgrDepthFiltered, bgrRoi);

	// calculate stats every 5. image
	if (m_imageSource->getSequence()%5 == 0) {
		if (m_statsList.size() == 30) {
			m_statsList.pop_front();
		}

		m_statsList.push_back(ImageStatistics(bgrDepthFiltered, true));
	}

//	const cv::Mat &megafilter = ColorSegmentation::m_statsFile.getProbabilityMapComplementary(bgrDepthFiltered, m_statsList, 0.00001);
//	if (ColorSegmentation::m_statsFile.getSampleCount()) {
//		WindowManager::getInstance().imShow("MEGAFILTER", megafilter);
//	}

	/*
	 * contours tracking based on depth
	 */
	cv::Mat bgrDepthFilteredGray;
	cv::Mat bgrDepthFilteredBinary;
	cvtColor(bgrDepthFiltered, bgrDepthFilteredGray, cv::COLOR_BGR2GRAY);
	cv::threshold(bgrDepthFilteredGray, bgrDepthFilteredBinary, 0, 255, 3);
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(bgrDepthFilteredBinary, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
	cv::Mat x;
	bgr.copyTo(x);
	for (int i = 0; i < contours.size(); i++) {
		cv::drawContours(x, contours, i, cv::Scalar(100,0,0), 2, 8);
	}
	/*
	 *
	 */

	WindowManager::getInstance().imShow("contours", x);
//	return;
//	const cv::Mat &bgrSaturated = processSaturate(bgr, 50);
//	WindowManager::getInstance().imShow("Saturated BGR",bgrSaturated);


	imageSourceArtificial->setColorMat(bgrRoi);

	processColorSegmentation(bgrRoi, depth);
}

void Processing::processColorSegmentation(const cv::Mat &bgr, const cv::Mat &depth)
{
	cv::Mat segmentedBGR;
	bgr.copyTo(segmentedBGR);
	cv::Mat bgrAveraged;
	cv::GaussianBlur(bgr, bgrAveraged, cv::Size(5,5), 10, 10);

//	const cv::Mat &probabilitiesOriginal = ColorSegmentation::m_statsFile.getProbabilityMapComplementary(bgrAveraged, m_statsList);
	const cv::Mat &probabilitiesOriginal = ColorSegmentation::m_statsFile.getProbabilityMap(bgrAveraged);
	cv::Mat probabilitiesDisplayable;
	probabilitiesOriginal.convertTo(probabilitiesDisplayable,CV_8UC3, 255, 1);
	WindowManager::getInstance().imShow("probabilities...", probabilitiesDisplayable);

	cv::Mat probabilities;
	cv::blur(probabilitiesOriginal, probabilities, cv::Size(5,5));

	for (int y = 0; y < bgr.rows; ++y) {
		for (int x = 0; x < bgr.cols; ++x) {
			float p = probabilities.at<float>(y,x);
			if (p > m_segmentation->getTMax()) {
				segmentedBGR.at<cv::Point3_<uint8_t> >(y,x) = cv::Point3_<uint8_t>(80,0,0);
//				probabilities.at<float>(y,x) = 1;
			} else if (p > m_segmentation->getTMin()) {
//				contains
				int y1 = std::max<int>(y-1,0);
				int y2 = std::min<int>(y+1,bgr.rows);
				int x1 = std::max<int>(x-1,0);
				int x2 = std::min<int>(x+1,bgr.cols);

				bool found = false;
				for (; y1 < y2 && !found; y1++) {
					for (; x1 < x2; x1++) {
						if (!x1 && !y1) continue;
						float p = probabilities.at<float>(y1,x1);
						if (p > m_segmentation->getTMax()) {
							found = true;
							goto G;
						}
					}
				}
				G:
				if (found) {
					segmentedBGR.at<cv::Point3_<uint8_t> >(y,x) = cv::Point3_<uint8_t>(80,80,0);
					probabilities.at<float>(y,x) = m_segmentation->getTMax();
				}
			}

		}
	}

	WindowManager::getInstance().imShow("Segmented BGR (with db)", segmentedBGR);
}
void Processing::processDepthFiltering(const cv::Mat &bgr, const cv::Mat &depth, cv::Mat &bgrDepthMasked, cv::Mat &bgrRoi)
{
	const double near = 500;
	const double far = 1200;
//	cv::Mat bgrDepthMasked;

	bgr.copyTo(bgrDepthMasked);
	filterDepth(bgrDepthMasked, depth, near, far);


	/**
	 * create mask consisting of points in depth choosen by near and far constants and their neighbours
	 * 1. remove undetected depth points
	 */
	cv::Mat tmp;
	depth.convertTo(tmp, CV_8UC1, 1.0/16.0, 0);
//	WindowManager::getInstance().imShow("tmp1", tmp);
	cv::Mat tmp23;
	for (int y = 0; y < depth.rows; ++y) {
		for (int x = 0; x < depth.cols; ++x) {
			uint8_t &pt = tmp.at<uint8_t>(y,x);
			if (!pt) {
				pt = 255;
			}
		}
	}

	cv::Mat tmp2;
	cv::threshold(tmp, tmp2, far/16, 255, cv::THRESH_BINARY_INV);
//	WindowManager::getInstance().imShow("tmp2", tmp2);
	cv::Mat tmp3, tmp4;
	cv::dilate(tmp2, tmp3, cv::Mat(), cv::Point(-1,-1), 15);
	cv::dilate(tmp2, tmp4, cv::Mat(), cv::Point(-1,-1), 3);
//	WindowManager::getInstance().imShow("tmp3", tmp3);

//	WindowManager::getInstance().imShow("bounds", tmp3-tmp4);
//	WindowManager::getInstance().imShow("BGR roi mask", tmp3);
	bgr.copyTo(bgrRoi,tmp3);

//	WindowManager::getInstance().imShow("BGR roi finished", bgrRoi);
}

void Processing::filterDepth(cv::Mat &dst, const cv::Mat &depth, int near, int far)
{
	assert(dst.rows == depth.rows);
	assert(dst.cols == depth.cols);

	for (int y = 0; y < dst.rows; ++y) {
		for (int x = 0; x < dst.cols; ++x) {
			const uint16_t val = depth.at<uint16_t>(y,x);

			if (val > near && val < far) {
				// keep color
			} else {
				dst.at<cv::Point3_<uint8_t> >(y,x) = cv::Point3_<uint8_t>(0,0,0);
			}
		}
	}
}

void iez::Processing::run()
{
	int sequence = 0;

	while (1) {
		if (!m_imageSource) {
			msleep(500);
			continue;
		}
		const cv::Mat &rgb = m_imageSource->getColorMat();

		cv::Mat bgr;
		cv::cvtColor(rgb, bgr,cv::COLOR_RGB2BGR);
		const cv::Mat &depth = m_imageSource->getDepthMat();

		process(bgr, depth);
		msleep(30);
	}
}
static int params[20]={11, 139, 0, 255, 95, 169, 174, 118};
void Processing::processHSVFilter(const cv::Mat &orig)
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

void Processing::keyPressEvent(QKeyEvent* keyEvent)
{
	switch (keyEvent->key()) {
	case Qt::Key_Escape:
		::exit(0);
		break;
	case Qt::Key_F:
		m_calculateHandTracker = true;
		break;
	}
}

void Processing::closeEvent()
{
	::exit(0);
}

cv::Mat Processing::processSaturate(const cv::Mat& bgr, const int satIncrease)
{
	cv::Mat hsv;
	cv::Mat bgrRet;
	cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
	for (int y = 0; y < bgr.rows; ++y) {
		for (int x = 0; x < bgr.cols; ++x) {
			cv::Point3_<uint8_t> &pt = hsv.at<cv::Point3_<uint8_t> >(y,x);
			if (int(pt.y)+satIncrease > 255) {
				pt.y = 255;
			} else {
				pt.y+=satIncrease;
			}
		}
	}
	cv::cvtColor(hsv, bgrRet, cv::COLOR_HSV2BGR);
	return bgrRet;
}
}
