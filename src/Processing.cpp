#include "Processing.h"
#include "ImageSource.h"
#include <stdint.h>
#include <assert.h>
#include <string>
#include <math.h>
//#include <QtGui>
//#include <QtCore>
//#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include "main.h"

//using namespace cv;
namespace iez {

Processing::Processing(ImageSourceBase *imgsrc)
:	m_imageSource(imgsrc)
,	m_calculateHandTracker(false)
,	m_statsList(0)
{
	moveToThread(&m_thread);
	m_segmentation = new ColorSegmentation();
	connect(imgsrc, SIGNAL(frameReceived()), this, SLOT(process()));
	m_thread.start();
}


Processing::~Processing(void)
{
	delete m_segmentation;
}


void Processing::process()
{
	uint32_t t1 = clock();
	const cv::Mat &depth = m_imageSource->getDepthMat();
	const cv::Mat &rgb = m_imageSource->getColorMat();
	cv::Mat bgr;
	cv::cvtColor(rgb, bgr,cv::COLOR_RGB2BGR);

//	qDebug("%d %d %d %d ", bgr.rows, depth.rows, bgr.cols, depth.cols);
	assert(bgr.rows == depth.rows);
	assert(bgr.cols == depth.cols);

	WindowManager::getInstance().imShow("Original", bgr);
	cv::Mat bgrRoi;
	cv::Mat bgrDepthFiltered;
	cv::Mat o,mask;



	processDepthFiltering(bgr, depth, o, bgrRoi);
	bgr.copyTo(bgrDepthFiltered);
	filterDepth(bgrDepthFiltered, depth);

	m_handTracker.process(bgr, depth);


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
	m_fps.tick();
//	processContourTracing(bgr, depth, bgrDepthFiltered);
	qDebug("fps: %2.1f", m_fps.fps());
//	return;
//	const cv::Mat &bgrSaturated = processSaturate(bgr, 50);
//	WindowManager::getInstance().imShow("Saturated BGR",bgrSaturated);


	imageSourceArtificial->setColorMat(bgrRoi);

//	processColorSegmentation(bgr, depth);
}

void Processing::processContourTracing(const cv::Mat &bgr, const cv::Mat &depth, const cv::Mat &bgrDepthFiltered)
{
/*
	 * contours tracking based on depth
	 */
	cv::Mat bgrDepthFilteredGray;
	cv::Mat bgrDepthFilteredBinary;
	cvtColor(bgrDepthFiltered, bgrDepthFilteredGray, cv::COLOR_BGR2GRAY);
	cv::threshold(bgrDepthFilteredGray, bgrDepthFilteredBinary, 0, 255, 3);
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(bgrDepthFilteredBinary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	qDebug("~~~~~~~~~~~~~~~processContourTracing~~~~~~~~~~~~~~~~");
	/*
	 *
	 */
	//hull Points
	std::vector<std::vector<cv::Point> > hullPoints(contours.size());
	//hull Indices
//	std::vector<std::vector<int> > hullIndices(contours.size());
	//defects
	std::vector<std::vector<cv::Vec4i> > defects(contours.size());

	for (int i = 0; i < contours.size(); i++) {
//		std::vector<int> hiall;
		cv::convexHull(contours[i], hullPoints[i]);
//		hullPoints[i] = std::vector<cv::Point> (ptlist.begin(), ptlist.end());
//		hullIndices[i] = std::vector<int> (hilist.begin(), hilist.end());
		continue;
//		qDebug("----- %d", hiall.size());
//		std::list<cv::Point> ptlist;
//		std::list<int> hilist;
//
//
//		for (int j = 0; j < hiall.size(); j++) {
//
//
//			qDebug("%d[%d]", contours[i].size(), hiall[j]);
//
//			cv::Point ptCurr = contours[i][hiall[j]];
//
//			cv::Point ptNext;
//			if (j+1 == hiall.size()) {
//				ptNext = contours[i][hiall[0]];
//			} else {
//				ptNext = contours[i][hiall[j+1]];
//			}
//			cv::Point ptPrev;
//			if (j == 0) {
//				ptPrev = contours[i][hiall[hiall.size()-1]];
//			} else {
//				ptPrev = contours[i][hiall[j-1]];
//			}
//
//			cv::Point diff = ptNext - ptCurr;
//			if (sqrt(diff.dot(diff)) > 0) {
//				ptlist.push_back(ptCurr);
//				hilist.push_back(hiall[j]);
//			}
//		}
//
//		hullPoints[i] = std::vector<cv::Point> (ptlist.begin(), ptlist.end());
//		hullIndices[i] = std::vector<int> (hilist.begin(), hilist.end());
//		if (hullIndices[i].size() > 2) {
//			cv::convexityDefects(contours[i], hullIndices[i], defects[i]);
//		}
//		qDebug("~~~~~ %d", defects[i].size());
	}


	cv::Mat contourOutput;
	bgr.copyTo(contourOutput);
	cv::Mat x;
	x.create(contourOutput.rows, contourOutput.cols, CV_8UC1);
//	x = 0;
	int maxindex = 0;
	int maxsize = 0;

	cv::Point center;
	for (int i = 0; i < contours.size(); i++) {
		qDebug("%ld ", hullPoints[i].size());
		if (hullPoints[i].size() > maxsize) {
			maxsize = hullPoints[i].size();
			maxindex = i;
		}
		cv::drawContours(contourOutput, contours, i, cv::Scalar(100,0,0), 1, 8);
		cv::drawContours(contourOutput, hullPoints, i, cv::Scalar(100,100,0), 1, 8);



		cv::drawContours(x, hullPoints, i, cv::Scalar(255), 1, 8);
		cv::Point center = calculateMeanIndices(x);
		cv::circle(contourOutput, center, 2, cv::Scalar(0,0,100), 2, 8);


		//qDebug("HullPoints: %ld", hullPoints[i].size());
//		std::vector<cv::Vec4i> &defa = defects[i];
//		std::vector<int> def(defa.size());
//		for (int j = 0; j < def.size(); j++) {
//			def[j] = defa[j][2];
//		}
//		std::sort(def.begin(), def.end());
//		std::list<cv::Point> contourDefectsList;
//		if (def.size() < 2) {
//			continue;
//		}
//		contourDefectsList.push_back(contours[i][def[0]]);
//		for (int j = 1; j < def.size(); j++) {
//			if (def[j] > def[j-1]) {
//				const cv::Point &defPoint = contours[i][def[j]];
//				contourDefectsList.push_back(defPoint);
//			}
//		}
//		std::vector<cv::Point> contourDefectsVector(contourDefectsList.begin(), contourDefectsList.end());
//		std::vector<std::vector<cv::Point> > contourDefectsSingleArray(1, contourDefectsVector);
//		cv::drawContours(contourOutput, contourDefectsSingleArray, 0, cv::Scalar(0,100,250), 1, cv::LINE_AA);

//		cv::circle(contourOutput, calculateMean(contourDefectsVector), 2, cv::Scalar(50,0,10), 2, cv::LINE_AA);
	}
	if (contours.size()) {
//		qDebug("MAX: %ld", maxindex);
		cv::Mat c;
		c.create(bgr.rows, bgr.cols, CV_8UC1);
		c = cv::Mat::zeros(bgr.rows, bgr.cols, CV_8UC1);;
		cv::drawContours(c, hullPoints, maxindex, cv::Scalar(255), 1, 8);

		// center of gravity
		cv::Point center = calculateMeanIndices(c);
		processContourPoints(bgr, depth,  contours[maxindex]);
	}


	WindowManager::getInstance().imShow("contours", contourOutput);
//	WindowManager::getInstance().imShow("contoursx", x);


}
void Processing::processColorSegmentation(const cv::Mat &bgr, const cv::Mat &depth)
{
	cv::Mat segmentedBGR;
	bgr.copyTo(segmentedBGR);
	cv::Mat bgrAveraged;
	cv::GaussianBlur(bgr, bgrAveraged, cv::Size(5,5), 10, 10);

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
				int y1 = std::max<int>(y-1, 0);
				int y2 = std::min<int>(y+1, bgr.rows);
				int x1 = std::max<int>(x-1, 0);
				int x2 = std::min<int>(x+1, bgr.cols);

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
void Processing::processDepthFiltering(const cv::Mat &bgr, const cv::Mat &depth, cv::Mat &bgrDepthMasked, cv::Mat &bgrRoi, int near)
{

	bgr.copyTo(bgrDepthMasked);

	int far = near + 100;

	filterDepth(bgrDepthMasked, depth, near, far);

	cv::Mat depthShowable;
	depth.convertTo(depthShowable, CV_8UC1, 1/18.0, 0);
//	WindowManager::getInstance().imShow("depth BW", depthShowable);
	WindowManager::getInstance().imShow("depth masked BGR", bgrDepthMasked);

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
	cv::dilate(tmp2, tmp3, cv::Mat(), cv::Point(-1,-1), 4);// dilate main
	cv::dilate(tmp2, tmp4, cv::Mat(), cv::Point(-1,-1), 3);// dilate border


//	WindowManager::getInstance().imShow("bounds", tmp3-tmp4);
//	WindowManager::getInstance().imShow("BGR roi mask", tmp3);
	bgr.copyTo(bgrRoi,tmp3);

//	WindowManager::getInstance().imShow("BGR roi finished", bgrRoi);
}

int Processing::findMin(const cv::Mat& depth)
{
	int minDepth = 10000;

	for (int i = 0; i < depth.rows; i++) {
		for (int j = 0; j < depth.cols; j++) {
			const int d = depth.at<uint16_t>(i, j);
			if (d) {
				minDepth = qMin<int>(minDepth, d);
			}
		}
	}

	return minDepth;
}

void Processing::filterDepth(cv::Mat &dst, const cv::Mat &depth, int near, int far)
{
	assert(dst.rows == depth.rows);
	assert(dst.cols == depth.cols);

	if (near == -1) {
		near = findMin(depth);
		far = near + 170;
	}
	const cv::Mat &mask = filterDepth2(depth, near, far);
	WindowManager::getInstance().imShow("x", mask);
	cv::Mat dst2;

	dst.copyTo(dst2, mask);
	WindowManager::getInstance().imShow("x2", dst2);
	dst2.copyTo(dst);
}

cv::Mat Processing::filterDepth2(const cv::Mat &depth, int near, int far)
{
	cv::Mat outputMask;
	outputMask.create(depth.rows, depth.cols, CV_8UC1);

	if (near == -1) {
		near = findMin(depth);
		far = near + 170;
	}

	for (int y = 0; y < depth.rows; ++y) {
		for (int x = 0; x < depth.cols; ++x) {
			const uint16_t val = depth.at<uint16_t>(y, x);

			if (val > near && val < far) {
				outputMask.at<uint8_t>(y, x) = 255;
			} else {
				outputMask.at<uint8_t>(y, x) = 0;
			}
		}
	}

	return outputMask;
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

cv::Point Processing::calculateMeanIndices(const cv::Mat &mat)
{
	if (mat.cols*mat.rows == 0) {
		return cv::Point(0,0);
	}

	cv::Point total(0,0);
	int wsum = 0;
	for (int i = 0; i < mat.rows; i++) {
		for (int j = 0; j < mat.cols; j++) {
			if (mat.at<uint8_t>(i,j)) {
				total = total + cv::Point(j,i);
				wsum++;
			}
		}
	}
	return cv::Point(total.x/wsum, total.y/wsum);
}

cv::Point Processing::calculateMean(const std::vector<cv::Point> &pointVector)
{
	if (pointVector.size() == 0) {
		return cv::Point(0,0);
	}

	cv::Point mean(0,0);
	float wsum = 0;
	for (int i = 0; i < pointVector.size(); i++) {
		mean = mean + pointVector[i];
	}
	int size = pointVector.size();
	return cv::Point(mean.x/size, mean.y/size);
}

cv::Point Processing::calculateWeightedMean(const std::vector<cv::Point> &pointVector)
{
	if (pointVector.size() == 0) {
		return cv::Point(0,0);
	}

	cv::Point2f mean(0,0);
	float wsum = 0;
	for (int i = 0; i < pointVector.size(); i++) {
		cv::Point diffPrev = pointVector[i] - pointVector[i?i-1:pointVector.size()-1];
		cv::Point diffNext = pointVector[i] - pointVector[(i==pointVector.size()-1)?i+1:0];

		float weight = sqrt(diffPrev.dot(diffPrev)) + sqrt(diffNext.dot(diffNext));
		qDebug("%+7.3f", weight);
		mean = mean + cv::Point2f(pointVector[i].x*weight, pointVector[i].y*weight);
		wsum += weight;
	}
	return cv::Point(mean.x/wsum, mean.y/wsum);
}

void Processing::processContourPoints(const cv::Mat &bgr, const cv::Mat &depth, const std::vector<cv::Point>& contour)
{
	cv::Mat output;
	bgr.copyTo(output);
	std::vector<double> coordx(contour.size());
	std::vector<double> coordy(contour.size());

	const std::vector<cv::Point> &contourSmoothed = smoothPoints(contour, 10);
	std::vector<int> hullSmoothedIndices;
	cv::convexHull(contourSmoothed, hullSmoothedIndices, false, false);
	qDebug("hull: %d", hullSmoothedIndices.size());
//	const std::vector<int> &fingersAll = fingerCandidates(contourSmoothed, hullSmoothedIndices);
	const std::vector<cv::Point> &fingersAll2 = fingerCandidates2(contourSmoothed, hullSmoothedIndices, depth);
//	const std::vector<int> &fingersSorted = categorizeFingers(contourSmoothed, fingersAll);

//	for (int i = 0; i < contourSmoothed.size(); i++) {
//		const cv::Point &prev = contourSmoothed[i?i-1:contourSmoothed.size()-1];
//		const cv::Point &next = contourSmoothed[(i!=contourSmoothed.size()-1)?i+1:0];
//		const cv::Point &curr = contourSmoothed[i];
//		const cv::Point tempPoint = prev+next;
//		const cv::Point midPoint = cv::Point(tempPoint.x/2, tempPoint.y/2);
//
//
//		float alfa = atan2(curr.x-prev.x, curr.y - prev.y);
//		float beta = atan2(next.x-curr.x, next.y - curr.y);
//		float gama = sin((alfa+beta)/2.0f);
//		float dist = pointDistance(curr, midPoint);
//		coordx[i] = curr.x/640.0f;//i/float(contourSmoothed.size());
//		coordy[i] = gama;
//	}
//	WindowManager::getInstance().plot("xx", coordx, coordy);
	cv::Mat x;
	x.create(bgr.rows, bgr.cols, CV_8UC1);
	x = cv::Mat::zeros(bgr.rows, bgr.cols, x.type());
	std::vector<std::vector<cv::Point> > arrContours(1, contourSmoothed);
	cv::drawContours(x, arrContours, 0, cv::Scalar(255), 1, CV_AA);
//	qDebug("~~~%d %d %d %d %d", fingersSorted[0], fingersSorted[1], fingersSorted[2],fingersSorted[3],fingersSorted[4]);
	for (int i = 0; i < fingersAll2.size(); i++) {

		cv::Point grr(fingersAll2[i]);//contourSmoothed[fingersAll[i]]);
		cv::circle(x, grr, 30, cv::Scalar(100));

//		cv::putText(x, std::to_string(i), cv::Point(contourSmoothed[fingersAll[fingersSorted[i]]]), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200));
		cv::circle(x, fingersAll2[i],10, cv::Scalar(100));
	}
	WindowManager::getInstance().imShow("contourSmooth", x);
//	msleep(1000);
}

float Processing::pointDistance(const cv::Point& pt1, const cv::Point& pt2)
{
	const cv::Point diff = pt1-pt2;
	return sqrtf(diff.dot(diff));
}

std::vector<cv::Point> Processing::smoothPoints(const std::vector<cv::Point>& vec, const int range)
{
//	assert(!(range&1));
//	assert(vec.size() <= range);
	if (range == 0 || vec.size() <= range) {
		return vec;
	}
	std::vector<cv::Point> ret(vec.size());

	for (int i = 0; i < vec.size(); i++) {

		std::vector<cv::Point> around(range*2+1);
		for (int j = -range; j < 0; j++) {
			around[j+range] = vec[(i+j >= 0)?i+j:i+j+vec.size()];
		}
		around[range] = vec[i];
		for (int j = 1; j <= range; j++) {
			around[j+range] = vec[(i+j < vec.size())?i+j:i+j-vec.size()];
		}

		ret[i] = calculateMean(around);
	}

	return ret;
}

std::vector<int> Processing::fingerCandidates(
		const std::vector<cv::Point>& contour,
		const std::vector<int>& hullIndices)
{
	std::list<cv::Point> defectList;
	std::list<int> indicesList;
	defectList.push_back(contour[hullIndices[0]]);
	cv::Point lastFalse(0,0);
	for (int i = 1; i < hullIndices.size()-1; i++) {
		const int prevIndex = hullIndices[i?i-1:hullIndices.size()-1];
		const int nextIndex = hullIndices[(i!=hullIndices.size()-1)?i+1:0];
		const cv::Point &prev = contour[prevIndex];
		const cv::Point &next = contour[nextIndex];
		const cv::Point &curr = contour[hullIndices[i]];
		const cv::Point tempPoint = prev+next;
		const cv::Point midPoint = cv::Point(tempPoint.x/2, tempPoint.y/2);


		if (Processing::pointDistance(curr, defectList.back()) > 15) {
			defectList.push_back(contour[hullIndices[i]]);
			indicesList.push_back(hullIndices[i]);
		} else {
			lastFalse = curr;
		}
	}
	struct C{
		C(const std::vector<cv::Point>& ax):cont(ax){};
		const std::vector<cv::Point>& cont;
		bool operator() (int a, int b) {return cont[a].y < cont[b].y;};
	} compare(contour);

	std::vector<int> candidates(indicesList.begin(),  indicesList.end());
	std::sort(candidates.begin(), candidates.end(), compare);


	struct C2{
		C2(const std::vector<cv::Point>& ax):cont(ax){};
		const std::vector<cv::Point>& cont;
		bool operator() (int a, int b) {return a<b;return cont[a].x < cont[b].x;};
	} compare2(contour);
	std::vector<int> candidates5(candidates.begin(), candidates.begin()+((candidates.size()>=5)?5:candidates.size()));
	std::sort(candidates5.begin(), candidates5.end(), compare2);
//
	std::vector<int> ret(candidates5.size());
	for (int i = 0; i < candidates5.size(); i++) {
		ret[i] = candidates5[i];
	}
	return ret;
}
struct Bag {
	Bag(const cv::Point& pt, const float area)
	:	m_area(area)
	{
		points.push_back(pt);
		calculateMean();
	}
	bool add(const cv::Point& pt)
	{
		if (Processing::pointDistance(pt, mean()) < m_area) {
			points.push_back(pt);
			calculateMean();
			return true;
		}
		return false;
	}

	const cv::Point& mean() const
	{
		return m_mean;
	}
private:
	inline void calculateMean()
	{
		cv::Point temp;
		for (std::list<cv::Point>::const_iterator it = points.begin(); it != points.end(); ++it) {
			temp += *it;
		}
		m_mean = cv::Point(temp.x/points.size(), temp.y/points.size());
	}
	std::list<cv::Point> points;
	cv::Point m_mean;
	const float m_area;
};
std::vector<cv::Point> Processing::fingerCandidates2(
		const std::vector<cv::Point>& contour,
		const std::vector<int>& hullIndices,
		const cv::Mat &depth)
{


	std::list<Bag> bagList;
	for (std::vector<int>::const_iterator it = hullIndices.begin(); it != hullIndices.end(); ++it) {

//		qDebug("%d %d", contour[*it].x, contour[*it].y);
		bool added = false;
		for (std::list<Bag>::iterator it2 = bagList.begin(); it2 != bagList.end(); ++it2) {
			if (it2->add(contour[*it])) {
				added = true;
				break;
			}
		}
		if (!added || bagList.empty()) {
			bagList.push_back(Bag(contour[*it], 12));
		}
	}

//	std::vector<std::pair<uint16_t, const Bag*> > bagPointDepthList(bagList.size());
//	// bagList finished
//	int i,j;
//	i = 0;
//	j = 0;
//	std::list<Bag> depthBagList;
//	for (std::list<Bag>::const_iterator bagit = bagList.begin(); bagit != bagList.end(); ++bagit) {
//		const uint16_t pointDepth = depth.at<uint16_t>(bagit->mean());
//		bool added = false;
//		for (std::list<Bag>::iterator it2 = depthBagList.begin(); it2 != depthBagList.end(); ++it2) {
//			if (it2->add(bagit->mean())) {
//				added = true;
//				break;
//			}
//		}
//
//		if (!added || bagList.empty()) {
//			depthBagList.push_back(Bag(bagit->mean(), 50));
//		}
//
//		bagPointDepthList[i].first = pointDepth;
//		bagPointDepthList[i].second = &(*bagit);
//		i++;
//	}

	// reformat to vector
	std::vector<cv::Point> ret(bagList.size());
	int i = 0;
	for (std::list<Bag>::const_iterator bagit = bagList.begin(); bagit != bagList.end(); ++bagit) {
		ret[i++] = bagit->mean();
	}

	return ret;
//
//	std::list<cv::Point> defectList;
//	std::list<int> indicesList;
//	defectList.push_back(contour[hullIndices[0]]);
//	cv::Point lastFalse(0,0);
//	for (int i = 1; i < hullIndices.size()-1; i++) {
//		const int prevIndex = hullIndices[i?i-1:hullIndices.size()-1];
//		const int nextIndex = hullIndices[(i!=hullIndices.size()-1)?i+1:0];
//		const cv::Point &prev = contour[prevIndex];
//		const cv::Point &next = contour[nextIndex];
//		const cv::Point &curr = contour[hullIndices[i]];
//		const cv::Point tempPoint = prev+next;
//		const cv::Point midPoint = cv::Point(tempPoint.x/2, tempPoint.y/2);
//
//
//		if (pointDistance(curr, defectList.back()) > 15) {
//			defectList.push_back(contour[hullIndices[i]]);
//			indicesList.push_back(hullIndices[i]);
//		} else {
//			lastFalse = curr;
//		}
//	}
//	struct C{
//		C(const std::vector<cv::Point>& ax):cont(ax){};
//		const std::vector<cv::Point>& cont;
//			bool operator() (int a, int b) {return cont[a].y < cont[b].y;};
//	} compare(contour);
//
//	std::vector<int> candidates(indicesList.begin(),  indicesList.end());
//	std::sort(candidates.begin(), candidates.end(), compare);
//
//
//	struct C2{
//		C2(const std::vector<cv::Point>& ax):cont(ax){};
//		const std::vector<cv::Point>& cont;
//			bool operator() (int a, int b) {return a<b;return cont[a].x < cont[b].x;};
//	} compare2(contour);
//	std::vector<int> candidates5(candidates.begin(), candidates.begin()+((candidates.size()>=5)?5:candidates.size()));
//	std::sort(candidates5.begin(), candidates5.end(), compare2);
////
//	std::vector<int> ret(candidates5.size());
//	for (int i = 0; i < candidates5.size(); i++) {
//		ret[i] = candidates5[i];
//	}
//	return ret;
}

std::vector<int> Processing::categorizeFingers(const std::vector<cv::Point>& contour, const std::vector<int> &candidates)
{
	std::vector<int> categorized(5);
	// 5points


	float maxDistance = 0;
	int maxIndex1 = 0;
	int maxIndex2 = 0;
	// thumb and pinky finger
	for (int i = 0; i < candidates.size(); i++) {
		int j = (i+1 ==candidates.size())?0:i+1;
			float dist = std::min<float>(abs(candidates[i] - candidates[j]), contour.size() - abs(candidates[i] - candidates[j]));
			if (maxDistance < dist) {
				maxIndex1 = i;
				maxIndex2 = j;
				maxDistance = dist;
			}
	}
	qDebug("->%d %d", maxIndex1, maxIndex2);
	// Point distance
	float minDistFur5 = 1000;
	int minDistFur5Index;
	int nextIndex1;
	int nextIndex2;
	nextIndex1 = (maxIndex1 + 1 == candidates.size())?0:maxIndex1+1;
	nextIndex2 = (maxIndex2 - 1 >= 0)?maxIndex2-1:0;

	if (nextIndex1 == maxIndex2) {
		nextIndex1 = (maxIndex1 - 1 >= 0)?maxIndex1-1:0;
		nextIndex2 = (maxIndex2 + 1 == candidates.size())?0:maxIndex2+1;
	}
//	int nextIndex2 = (maxIndex2 + 1 == candidates.size())?0:maxIndex2+1;

	nextIndex2 = (maxIndex2 - 1 >= 0)?maxIndex2-1:0;

	if (pointDistance(contour[maxIndex1], contour[nextIndex1]) < pointDistance(contour[maxIndex2], contour[nextIndex2])) {
		categorized[0] = maxIndex2;//thumb
		categorized[1] = nextIndex2;//pointer
		categorized[2] = nextIndex2+1;//middle
		categorized[3] = nextIndex1;// ring
		categorized[4] = maxIndex1;//Pinky
	} else {
		categorized[0] = maxIndex1;//thumb
		categorized[1] = nextIndex1;//pointer
		categorized[2] = nextIndex1+1;//middle
		categorized[3] = nextIndex2;// ring
		categorized[4] = maxIndex2;//Pinky
	}
//	if (dist < minDistFur5) {
//		minDistFur5 = dist;
//		minDistFur5Index = prevIndex1;
//	}
//	for (int i = 0; i < candidates.size(); i++) {
//		if (i != maxIndex1 && i != maxIndex2) {
//
//		}

//	categorized[0];
//	msleep(2000);
	return categorized;
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
