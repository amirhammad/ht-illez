#include "HandTracker.h"
#include "Processing.h"

#include <assert.h>
#include <vector>
#include <limits>


namespace iez {

#define NEXT_HAND_TOLERANCE (5)
#define HAND_MAX_PHYSICAL_DEPTH	(200)
#define C3	(90)

int HandTracker::Worker::m_runningThreads = 0;

void HandTracker::distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT)
{
	cv::distanceTransform(binaryHandFiltered, handDT, CV_DIST_L2, 3);
}

void HandTracker::findHandCenter(const cv::Mat &handDT,	 cv::Point &maxDTPoint)
{
	maxDTPoint = cv::Point(-1, -1);
	float max = 0;
	for (int i = 0; i < handDT.rows; i++) {
		for (int j = 0; j < handDT.cols; j++) {
			const float d = handDT.at<float>(i, j);
			if (d > max) {
				max = d;
				maxDTPoint = cv::Point(j, i);
			}
		}
	}
}

float HandTracker::findHandCenterRadius(const cv::Mat& binaryHandFiltered,
		const cv::Point& maxDTPoint, const std::vector<cv::Point> contour)
{
	float minDistance = binaryHandFiltered.rows*binaryHandFiltered.cols;

	for (int i = 0; i < contour.size(); i++) {
		float dist = Processing::pointDistance(maxDTPoint, contour[i]);
		if ( dist < minDistance) {
			minDistance = dist;
		}
	}
	return minDistance;
}


#define PALM_RADIUS_RATIO (1.2f)
void HandTracker::findPalm(cv::Mat &binaryPalmMask,
						   std::vector<cv::Point> &palmContour,
						   const cv::Mat &binaryHand,
						   const std::vector<cv::Point> &contour,
						   const cv::Point &palmCenter,
						   const float palmRadius)
{
	const int maxValues = 100;


	QList<cv::Point> boundaryPointList;
	boundaryPointList.reserve(maxValues);

	QStringList k;

	for (float randomAngleDeg = 0; randomAngleDeg < 360.0f; ) {
		cv::Point randomPoint;
		randomPoint.x = palmRadius*PALM_RADIUS_RATIO*cosf(randomAngleDeg*2*M_PI/360.f);
		randomPoint.y = palmRadius*PALM_RADIUS_RATIO*sinf(randomAngleDeg*2*M_PI/360.f);
		randomPoint += palmCenter;

		if ((randomPoint.y < 0 && randomPoint.y >= binaryHand.rows)
		&& (randomPoint.x < 0 && randomPoint.x >= binaryHand.cols)) {
			continue;
		}
		cv::Point nearestPoint = Processing::findNearestPoint(contour, randomPoint);
		if (!boundaryPointList.isEmpty()) {
			const float d = Processing::pointDistance(nearestPoint, boundaryPointList.last());
			if (d < 10.0f) {
				k << QString::number(randomAngleDeg).append("-").append(QString::number(d));
			} else {
				boundaryPointList.append(nearestPoint);
			}
		} else {
			boundaryPointList.append(nearestPoint);
		}

		randomAngleDeg += ((static_cast<unsigned int>(qrand())%(360*1000))/1000.f)/maxValues;
	}
//	qDebug() << "---->---->" << k.size() << boundaryPointList.size();// << k;


	/// produce output

	binaryPalmMask = cv::Mat::zeros(binaryHand.rows, binaryHand.cols, CV_8UC1);
	if (!boundaryPointList.size()) {
		return;
	}

	std::vector<std::vector<cv::Point> > hc(1);
	palmContour = boundaryPointList.toVector().toStdVector();
	hc[0] = palmContour;

	cv::fillPoly(binaryPalmMask, hc, cv::Scalar_<uint8_t>(255));
}

void HandTracker::findFingers(cv::Mat &binaryFingersMask,
							  const std::vector<cv::Point> &palmContour,
							  const cv::Mat &binaryHand,
							  const cv::Mat &palmMask,
							  Data &data)
{
	std::pair<cv::Point, cv::Point> wristPointPair;

	if (palmContour.size() <= 2) {
		qDebug("Error: palmContour.size() <= 2");
		return;
	}
	/// find wristPointPair
	{
		float maxDist = std::numeric_limits<float>::min();

		QLinkedList<wristpair> wpp3;

		const float dextra = Processing::pointDistance(palmContour[0], palmContour[palmContour.size() - 1]);
		if (maxDist < dextra) {
			maxDist = dextra;
			wristpair wpp;
			wpp.first = palmContour[0];
			wpp.second = palmContour[palmContour.size() - 1];
			wpp3.append(wpp);
		}

		for (int i = 0; i < palmContour.size() - 1; i++) {
			const float d = Processing::pointDistance(palmContour[i], palmContour[i + 1]);
			if (maxDist < d) {
				maxDist = d;
				wristpair wpp;
				wpp.first = palmContour[i];
				wpp.second = palmContour[i + 1];
				wpp3.append(wpp);
				if (wpp3.size() > 3) {
					wpp3.removeFirst();
				}
			}
		}
		wristpair wppData = data.wrist();
		cv::Point wppMeanData = Processing::pointMean(wppData.first, wppData.second);

		wristpair wppFinal;
		float minDist = std::numeric_limits<float>::max();
		while (wpp3.size()) {
			wristpair wpp = wpp3.last();
			cv::Point wppMean = Processing::pointMean(wpp.first, wpp.second);
			float d = Processing::pointDistance(wppMean, wppMeanData);

			//using physical depth instead of ~size of hand... it is equal..
			if (minDist > d && ( wppMeanData == cv::Point(0, 0) || d < HAND_MAX_PHYSICAL_DEPTH)) {
				minDist = d;
				wppFinal = wpp;
			}
			wpp3.removeLast();
		}

		// lowpassing

		if (Processing::pointDistance(wppData.first, wppFinal.first) > Processing::pointDistance(wppData.first, wppFinal.second)) {
			// swap
			wppFinal = wristpair(wppFinal.second, wppFinal.first);
		}
		const float ratio = 0.8f;
		wristPointPair = wristpair(Processing::pointMean(wppFinal.first, wppData.first, ratio),
								   Processing::pointMean(wppFinal.second, wppData.second, ratio));
		// ~lowpassing

//		wristPointPair = wppFinal;

		data.setWrist(wristPointPair);
	}

	{




		cv::Mat tmp;
		cv::dilate(palmMask, tmp, cv::Mat(), cv::Point(-1,-1), 2);// dilate main
		cv::Mat binaryFingersDiff = binaryHand - tmp;
		std::vector<std::vector<cv::Point> > contours;
		cv::findContours(binaryFingersDiff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

//		cv::Mat contoursAndWristMat = cv::Mat::zeros(binaryHand.rows, binaryHand.cols, CV_8UC1);

//		for (int i = 0; i < contours.size(); i++) {
//			cv::drawContours(contoursAndWristMat, contours, i, cv::Scalar(100), 2);
//		}
//		wristpair wrist = data.wrist();
//		cv::line(contoursAndWristMat, wrist.first, wrist.second, cv::Scalar(200), 5);

		/// Find contour, that is closest to wrist and remove it

		cv::Point wristPointPairMean = Processing::pointMean(wristPointPair.first, wristPointPair.second);
		float minDist = std::numeric_limits<float>::max();
		int minIndex = -1;
		for (int i = 0; i < contours.size(); i++) {
//			cv::contourArea(contours[i]);
			cv::Point center = Processing::calculateMean(contours[i]);
			float d = Processing::pointDistance(center, wristPointPairMean);
			if (minDist > d) {
				minDist = d;
				minIndex = i;
			}
		}

		if (minIndex == -1 || contours.size() == 1) {
			qDebug("minindex = -1");
			return;
		}

		std::vector<std::vector<cv::Point> > contoursNoWrist(contours.size() - 1);
		for (int i = 0; i < minIndex; i++) {
			contoursNoWrist[i] = contours[i];
		}
		for (int i = minIndex + 1; i < contours.size(); i++) {
			contoursNoWrist[i - 1] = contours[i];
		}

		/// draw fingers from contours

		binaryFingersMask = cv::Mat::zeros(binaryHand.rows, binaryHand.cols, CV_8UC1);
		cv::fillPoly(binaryFingersMask, contoursNoWrist, cv::Scalar_<uint8_t>(255));


//		binaryFingersDiff;

//		binaryFingersMask = binaryFingersDiff;
	}

//	binaryFingersMask = binaryHand - tmp;
}

HandTracker::HandTracker()
{
	m_maxThreads = 3;
}

void HandTracker::invokeProcess(const cv::Mat &bgr, const cv::Mat &depth, const int imageId)
{
	if (Worker::runningThreads() < m_maxThreads) {
		Worker *w = new Worker(bgr, depth, imageId, m_data);
		QThreadPool::globalInstance()->start(w);
	} else {
		qDebug() << "cannot start thread";
	}
}

void HandTracker::process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId, Data &data)
{
	QTime t;
	t.start();
	/// Create binary hand

	cv::Mat binaryHand;
	std::vector<cv::Point> handContour;
	cv::Mat candidates;

	cv::Point referencePoint; // used for selection of hand from multiple components (nearest)

	bool isReferenceImage = true;


	int near;
	int far;
	if (isReferenceImage) {
//		qDebug("%d referenceImage", imageId);
		// we had not image for long time...
		near = Processing::findMin2(depth, referencePoint);
		far = near + HAND_MAX_PHYSICAL_DEPTH;
	} else {
//		referencePoint = m_prevHandPosition;
//		near = m_prevHandDepth - NEXT_HAND_TOLERANCE;
//		far = near + HAND_MAX_PHYSICAL_DEPTH + NEXT_HAND_TOLERANCE;
	}

	candidates = Processing::filterDepth2(depth, near, far);

	// mask with rectangle
//	cv::Mat mask = cv::Mat::zeros(depth.rows, depth.cols, CV_8UC1);
//	cv::Point topLeft(m_prevHandPosition.x - C3, m_prevHandPosition.y - C3);
//	cv::Point bottomRight(m_prevHandPosition.x + C3, m_prevHandPosition.y + C3);

//	cv::rectangle(mask, topLeft, bottomRight, cv::Scalar_<uint8_t>(255), CV_FILLED);
//	cv::Mat f;
//	candidates.copyTo(f, mask);
//	f.copyTo(candidates);
//	WindowManager::getInstance().imShow("xxxx", candidates);


	/// Filter
	cv::Mat candidatesFiltered;
	cv::medianBlur(candidates, candidatesFiltered, 5);
//		cv::Mat tmp;
//		cv::erode(frontMostObjects, tmp, cv::Mat(), cv::Point(-1,-1), 2);// dilate main
//		cv::dilate(tmp, frontMostObjectsFiltered, cv::Mat(), cv::Point(-1,-1), 2);// dilate main

//		 fill
//		cv::floodFill(frontMostObjectsFiltered, frontMostPoint, cv::Scalar_<uint8_t>(50));

	/// Contours -> find single contour
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(candidatesFiltered, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	if (contours.size() == 0) {
		qDebug("%d Zero objects", imageId);

		if (!isReferenceImage) {
			process(bgr, depth, imageId, data);
		}
		return;
	} else if (contours.size() == 1) {
		handContour = contours[0];
	} else {
		float minDist = std::numeric_limits<float>::max();
		int minIndex = std::numeric_limits<int>::max();
		cv::Point mid;
		for (int i = 0; i < contours.size(); i++) {
			const cv::Point tmpmid = Processing::calculateMean(contours[i]);
			const float d = Processing::pointDistance(tmpmid, referencePoint);
			if (minDist > d) {
				minDist = d;
				minIndex = i;
				mid = tmpmid;
			}
		}
		handContour = contours[minIndex];

	}

	/// fill contour

	cv::Mat out = cv::Mat::zeros(depth.rows, depth.cols, CV_8UC1);
	std::vector<std::vector<cv::Point> > hc(1);
	hc[0] = handContour;
	cv::fillPoly(out, hc, cv::Scalar_<uint8_t>(255));

	if (isReferenceImage) {

	} else {
//		int minDepth = std::numeric_limits<int>::max();
//		for (int i = 0; i < out.rows; i++) {
//			for (int j = 0; j < out.cols; j++) {
//				if (out.at<uint8_t>(i, j)) {
//					int d = depth.at<uint16_t>(i, j);
//					if (d > near && minDepth > d) {
//						minDepth = d;
//					}
//				}
//			}
//		}
//		m_prevHandDepth = minDepth;

	}
	/// save binary hand
	out.copyTo(binaryHand);

	cv::ellipse(out, referencePoint, cv::Size(5, 5), 0, 0, 360, cv::Scalar_<uint8_t>(200), 2);
	WindowManager::getInstance().imShow("binaryHand", binaryHand);



	/// Distance Transform

	cv::Mat handDT;
	distanceTransform(binaryHand, handDT);

	/// Find hand center

	cv::Point palmCenter;
	findHandCenter(handDT, palmCenter);


	/// Find palm radius

	const float palmRadius = findHandCenterRadius(binaryHand, palmCenter, handContour);

	if (palmRadius <= 0) {
		return;
	}

	/// ready: handCenter, innerCircleRadius, binaryHand(filtered), handContour

	/// find palm mask
	cv::Mat palmMask;
	std::vector<cv::Point> palmContour;
	findPalm(palmMask, palmContour, binaryHand, handContour, palmCenter, palmRadius);


	/// isolate fingers, ready: +palmMask

	/// find wrist ... TODO
	cv::Mat fingersMask;
	findFingers(fingersMask, palmContour, binaryHand, palmMask, data);


	/// Draw

	cv::Mat centerHighlited = bgr.clone();

	wristpair wrist = data.wrist();
	cv::line(centerHighlited, wrist.first, wrist.second, cv::Scalar(100), 5);
	cv::ellipse(centerHighlited, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100, 0, 255), 10);


	WindowManager::getInstance().imShow("handCenter", centerHighlited);

	// draw only if valid, else results in flickering...
	if (fingersMask.rows == binaryHand.rows) {
		WindowManager::getInstance().imShow("fingers", fingersMask);
	}
//	WindowManager::getInstance().imShow("PALM", palmMask);

//	cv::Mat gray;
//	handDT.convertTo(gray, CV_8UC1, 10.0f);
//	WindowManager::getInstance().imShow("distanceTransform", handDT);
	qDebug("fps: %f", (1000/30.f)/t.elapsed()*30);
}

HandTracker::Worker::Worker(const cv::Mat bgr, const cv::Mat depth, const int imageId, Data &data)
:	m_bgr(bgr)
,	m_depth(depth)
,	m_imageId(imageId)
,	m_data(data)
{

}

void HandTracker::Worker::run()
{
	m_runningThreads++;
	process(m_bgr, m_depth, m_imageId, m_data);
	m_runningThreads--;
}

int HandTracker::Worker::runningThreads()
{
	return m_runningThreads;
}

void HandTracker::Data::setWrist(std::pair<cv::Point, cv::Point> wrist)
{
	QMutexLocker l(&m_mutex);
	m_wrist = wrist;
}

std::pair<cv::Point, cv::Point> HandTracker::Data::wrist() const
{
	QMutexLocker l(&m_mutex);
	return m_wrist;
}

} // Namespace iez
