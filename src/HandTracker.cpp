#include "HandTracker.h"
#include "Processing.h"

#include <assert.h>
#include <vector>
#include <limits>


namespace iez {

#define NEXT_HAND_TOLERANCE (5)
#define HAND_MAX_PHYSICAL_DEPTH	(200)
#define C3	(90)
#define PALM_RADIUS_RATIO (1.2f)

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

//		randomAngleDeg += ((static_cast<unsigned int>(qrand())%(360*1000))/1000.f)/maxValues;
		randomAngleDeg += 360.0f/maxValues;
	}
//	qDebug() << "---->---->" << k.size() << boundaryPointList.size();// << k;
	/// SORT by angle

	struct compare {
		compare(cv::Point palmCenter)
		:	m_palmCenter(palmCenter){

		}

		float getAngle(cv::Point point) {
			const float dy = point.y - m_palmCenter.y;
			const float dx = point.x - m_palmCenter.x;
			const float angle = atan2(dy, dx);
			return angle;
		}

		bool operator() (cv::Point a, cv::Point b) {
			return getAngle(a) < getAngle(b);
		}

		cv::Point m_palmCenter;
	};
	std::sort(boundaryPointList.begin(), boundaryPointList.end(), compare(palmCenter));


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
							  const cv::Point &handCenter,
							  Data &data)
{
	std::pair<cv::Point, cv::Point> wristPointPair;

	if (palmContour.size() <= 2) {
		qDebug("Error: palmContour.size() <= 2");
		return;
	}
	/// find wristPointPair
	{

		/// Find max distance between 2 successive points, add them to linked list of size at most 3

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

		/// Find wrist(out of 3 got in previous algorithm step) nearest to previously saved wrist

		wristpair wppData = data.wrist();
		cv::Point wppMeanData = Processing::pointMean(wppData.first, wppData.second);

		wristpair wppFinal = wppData;
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


		/// order by right-hand rule

		const cv::Point &pt1 = wppFinal.first;
		const cv::Point &pt2 = handCenter;
		const cv::Point &pt3 = wppFinal.second;
		const cv::Point normal(pt2.y - pt1.y, pt1.x - pt2.x); // (from pt1 to pt2) -> normal -90*
		if ((pt3 - pt1).dot(normal) < 0) {
			// swap
			wppFinal = wristpair(wppFinal.second, wppFinal.first);
		}

		/// lowpassing
		const float ratio = 0.8f;
		wristPointPair = wristpair(Processing::pointMean(wppFinal.first, wppData.first, ratio),
								   Processing::pointMean(wppFinal.second, wppData.second, ratio));

		/// save wristpair
		data.setWrist(wristPointPair);
	}

	{
		cv::Mat tmp;
		cv::dilate(palmMask, tmp, cv::Mat(), cv::Point(-1,-1), 2);// dilate main
		cv::Mat binaryFingersDiff = binaryHand - tmp;
		std::vector<std::vector<cv::Point> > contours;
		cv::findContours(binaryFingersDiff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		/// use dot product with normal to remove contures under wrist line (half-plane filter out)

		wristpair wrist = data.wrist();
		QList<std::vector<cv::Point> > l;
		for (int i = 0; i < contours.size(); i++) {
			cv::Point center = Processing::calculateMean(contours[i]);
			const cv::Point &pt1 = wrist.first;
			const cv::Point &pt2 = wrist.second;
			const cv::Point &pt3 = center;
			const cv::Point normal(pt2.y - pt1.y, pt1.x - pt2.x); // (from pt1 to pt2) -> normal -90*
			if ((pt3 - pt1).dot(normal) < 0) {
				l.append(contours[i]);
			}
		}
		std::vector<std::vector<cv::Point> > contoursNoWrist(l.size());
		QListIterator<std::vector<cv::Point> > it = l;
		int index = 0;
		while (it.hasNext()) {
			const std::vector<cv::Point> &contour = it.next();
			contoursNoWrist[index++] = contour;
		}


		/// draw fingers from contours

		binaryFingersMask = cv::Mat::zeros(binaryHand.rows, binaryHand.cols, CV_8UC1);
		cv::fillPoly(binaryFingersMask, contoursNoWrist, cv::Scalar_<uint8_t>(255));
	}
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

	candidates = Processing::filterDepthMask(depth, near, far);

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
	findFingers(fingersMask, palmContour, binaryHand, palmMask, palmCenter, data);
	cv::Mat fingerMaskOutput;
	fingersMask.copyTo(fingerMaskOutput);
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100), 1);
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius*PALM_RADIUS_RATIO, palmRadius*PALM_RADIUS_RATIO), 0, 0, 360, cv::Scalar(180), 1);
	cv::putText(fingerMaskOutput, QString::number(handContour.size()).toStdString(), cv::Point(10, 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
	/// Draw

	cv::Mat centerHighlited = bgr.clone();

	wristpair wrist = data.wrist();
	cv::line(centerHighlited, wrist.first, wrist.second, cv::Scalar(100), 5);
	cv::ellipse(centerHighlited, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100, 0, 255), 10);


	WindowManager::getInstance().imShow("handCenter", centerHighlited);

	// draw only if valid, else results in flickering...
	if (fingerMaskOutput.rows == binaryHand.rows) {
		WindowManager::getInstance().imShow("fingers", fingerMaskOutput);
	}
//	WindowManager::getInstance().imShow("PALM", palmMask);

//	cv::Mat gray;
//	handDT.convertTo(gray, CV_8UC1, 10.0f);
//	WindowManager::getInstance().imShow("distanceTransform", handDT);
	qDebug("ID=%5d fps: %4.1f ... %3dms", imageId, (1000/30.f)/t.elapsed()*30, t.elapsed());
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
