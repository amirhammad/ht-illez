#include "HandTracker.h"
#include "Processing.h"


#include <assert.h>
#include <vector>
#include <limits>
#include <queue>

namespace iez {

#define HAND_MAX_PHYSICAL_DEPTH	(200)
#define PALM_RADIUS_RATIO (1.7f)
#define MAX_THREADS (1)

// FINGER_FACTOR*PALM_RADIUS => minimal length of finger -- set to half the size of finger
#define FINGER_LENGTH_FACTOR (0.7f)

#define FINGER_MAXWIDTH_FACTOR (FINGER_LENGTH_FACTOR*0.9f)

QVector<float> * HandTracker::C1::m_vector = 0;

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

void HandTracker::findWrist(const std::vector<cv::Point> &palmContour,
							const cv::Point &handCenter,
							Data &data)
{
	std::pair<cv::Point, cv::Point> wristPointPair;

	if (palmContour.size() <= 2) {
		qDebug("Error: palmContour.size() <= 2");
		return;
	}

	/**
	 * construct the Priority queue and take out first 5 for consideration.
	 * Then choose closest one to previous data.wrist (if valid).
	 */

	/// calculate distances between neighbors
	QVector<float> distanceToNeighbor(palmContour.size());
	distanceToNeighbor[distanceToNeighbor.size() - 1] =
			Processing::pointDistance(palmContour[0], palmContour[palmContour.size() - 1]);
	for (int i = 0; i < palmContour.size() - 1; i++) {
		distanceToNeighbor[i] =
				Processing::pointDistance(palmContour[i], palmContour[i + 1]);
	}


	/// comparator for PQ


	C1::setVector(&distanceToNeighbor);

	/// construct the PQ
	std::priority_queue<int, std::vector<int>, C1> wpPq;
	for (int i = 0; i < distanceToNeighbor.size(); i++) {
		wpPq.push(i);
	}


	/// Find wrist(out of 5 got in previous algorithm step) nearest to previously saved wrist

	wristpair_t wppData = data.wrist();
	cv::Point wppMeanData = Processing::pointMean(wppData.first, wppData.second);

	wristpair_t wppFinal = wppData;
	float minDist = std::numeric_limits<float>::max();
	wppFinal = wristpair_t(palmContour[wpPq.top()], palmContour[(wpPq.top() + 1)%palmContour.size()]);

	int cnt = 0;
	while (wpPq.size() > 0 && cnt++ < 5) {
		wristpair_t wpp(palmContour[wpPq.top()], palmContour[(wpPq.top() + 1)%palmContour.size()]);
		cv::Point wppMean = Processing::pointMean(wpp.first, wpp.second);
		float d = Processing::pointDistance(wppMean, wppMeanData);

		//using physical depth instead of ~size of hand... it is equal..
		if (minDist > d && ( wppMeanData == cv::Point(0, 0) || d < HAND_MAX_PHYSICAL_DEPTH)) {
			minDist = d;
			wppFinal = wpp;
		}
		wpPq.pop();
	}



	/// order pair by right-hand rule

	const cv::Point &pt1 = wppFinal.first;
	const cv::Point &pt2 = handCenter;
	const cv::Point &pt3 = wppFinal.second;
	const cv::Point normal(pt2.y - pt1.y, pt1.x - pt2.x); // (from pt1 to pt2) -> normal -90*
	if ((pt3 - pt1).dot(normal) < 0) {
		// swap
		wppFinal = wristpair_t(wppFinal.second, wppFinal.first);
	}

	/// lowpassing
	const float ratio = 0.8f;
	wristPointPair = wristpair_t(Processing::pointMean(wppFinal.first, wppData.first, ratio),
							   Processing::pointMean(wppFinal.second, wppData.second, ratio));

	/// save wristpair
	data.setWrist(wristPointPair);
}


/*
 * returns one or more fingertips contained in bounding rectangle defined by rectPoints
 * TODO: more precise
 */
QList<cv::Point> HandTracker::findFingertip(const cv::RotatedRect &rotRect,
											const float palmRadius,
											const cv::Point &palmCenter)
{
	cv::Point2f rectPoints[4];
	rotRect.points(rectPoints);

	const float rectWidth = std::min(rotRect.size.width, rotRect.size.height);
	int fingerCount = rectWidth/(palmRadius*FINGER_MAXWIDTH_FACTOR) + 1;
	if (fingerCount == 0) {
		return QList<cv::Point>();
	} else if (fingerCount > 1) {
		// add factor 0.9, because when fingers are close to each other, they appear thinner
		fingerCount = rectWidth/(palmRadius*FINGER_MAXWIDTH_FACTOR*0.9f) + 1;
	}

	// find fingertips
	struct compare1 {
		compare1(cv::Point palmCenter)
		:	m_palmCenter(palmCenter){

		}

		int operator()(cv::Point2f p1, cv::Point2f p2) {
			float p1d = Processing::pointDistance(p1, m_palmCenter);
			float p2d = Processing::pointDistance(p2, m_palmCenter);
			return p1d < p2d;
		}
		cv::Point m_palmCenter;
	};

	std::vector<cv::Point2f> rectPointsVector(4);
	for (int i = 0; i < 4; i++) {
		rectPointsVector[i] = rectPoints[i];
	}
	std::sort(rectPointsVector.begin(), rectPointsVector.end(), compare1(palmCenter));
	QList<cv::Point> l;
	if (fingerCount == 1) {
		cv::Point fingertip = Processing::pointMean(rectPointsVector[2], rectPointsVector[3]);

		l.append(fingertip);
	} else {
		for (int i = 0; i < fingerCount; i++) {
			const float n1 = rectWidth/fingerCount;
			const float ratio = (i*(n1) + 0.5f*n1)/rectWidth;
			cv::Point fingertip = Processing::pointMean(rectPointsVector[2], rectPointsVector[3], ratio);
			l.append(fingertip);
		}
	}

	return l;


}
void HandTracker::findFingers(cv::Mat &binaryFingersMask,
							  std::vector<std::vector<cv::Point> > &fingersContours,
							  const cv::Mat &binaryHand,
							  const cv::Mat &palmMask,
							  Data &data)
{
	cv::Mat tmp;
	cv::dilate(palmMask, tmp, cv::Mat(), cv::Point(-1,-1), 2);// dilate main
	cv::Mat binaryFingersDiff = binaryHand - tmp;
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(binaryFingersDiff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	/// use dot product with normal to remove contures under wrist line (half-plane filter out)

	wristpair_t wrist = data.wrist();
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


	fingersContours = contoursNoWrist;
}

HandTracker::HandTracker()
{
	m_maxThreads = MAX_THREADS;
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
			if (contours[i].size() < 20) {
				continue;
			}
			if (minDist > d) {
				minDist = d;
				minIndex = i;
				mid = tmpmid;
			}
		}
		handContour = contours[minIndex];
	}
	if (handContour.size() == 0) {
		return;
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


	// ignore small palmContours
	float area = cv::contourArea(palmContour);
	if (area < 1000) {
		qDebug("contourAreaFail %f", area);
		return;
	}

	/// isolate fingers, ready: +palmMask

	/// find wrist

	findWrist(palmContour, palmCenter, data);


	/// find fingers

	cv::Mat fingersMask;
        std::vector<std::vector<cv::Point> > fingersContours;
	findFingers(fingersMask, fingersContours, binaryHand, palmMask, data);




	cv::Mat fingerMaskOutput;
	fingersMask.copyTo(fingerMaskOutput);


	/// ~~~~
	QList<cv::Point> fingertips;
	for (int i = 0; i < fingersContours.size(); i++) {
		cv::RotatedRect rotRect = cv::minAreaRect(fingersContours[i]);

		// filter out short fingers
		if (qMax(rotRect.size.height, rotRect.size.width) < palmRadius * FINGER_LENGTH_FACTOR) {
			continue;
		}
		cv::Point2f rect_points[4];
		rotRect.points(rect_points);
		for( int j = 0; j < 4; j++ ) {
			cv::line(fingerMaskOutput, rect_points[j], rect_points[(j+1)%4], cv::Scalar(100), 2, 8 );
		}

		{
			// unmerge merged fingertips

//				QList<cv::Point2f> unmergedFingertips;
//				for (int i = 0; i < 4; i++) {
//					unmergedFingertips.append(rect_points[i]);
//				}
			QList<cv::Point> rectFingertips = findFingertip(rotRect, palmRadius, palmCenter);
			foreach (cv::Point fingertip, rectFingertips) {
				fingertips.append(fingertip);
//					cv::ellipse(fingerMaskOutput, fingertip, cv::Size(2, 2), 0, 0, 360, cv::Scalar(100), 5);
			}
		}
		data.setFingertips(fingertips);
	}

	{

		/// recognizer
		data.pose()->categorize(data.wrist(), data.fingertips());
	}
	///

/*
//		{
//			// find palm line

//			// rotation of hand
//			const float angleRad =
//					atan2f(
//						data.wrist().first.y - data.wrist().second.y,
//						data.wrist().first.x - data.wrist().second.x);
//			const float angleDeg = angleRad	* (180.0f / ((float)M_PI));

//			cv::Mat binaryHandRotated;
//			Processing::rotate(binaryHand, angleDeg, binaryHandRotated);
//			wristpair w = data.wrist();
//			const cv::Point offset(binaryHandRotated.cols/2, binaryHandRotated.rows/2);
//			w.first = w.first - offset;
//			w.second = w.second - offset;
//			const cv::Point p1 = cv::Point(cosf(angleRad)*w.first.x + sinf(angleRad)*w.first.y,
//										   - sinf(angleRad)*w.first.x + cosf(angleRad)*w.first.y) + offset;
//			const cv::Point p2 = cv::Point(cosf(angleRad)*w.second.x + sinf(angleRad)*w.second.y,
//										   - sinf(angleRad)*w.second.x + cosf(angleRad)*w.second.y) + offset;
////			cv::line(binaryHandRotated, p1, p2, cv::Scalar(100), 2, 8 );
//			for (int i = p1.y; i >= 0; i--) {
//				int edge1 = -1;
//				int edge2 = -1;
//				wristpair palmLine;
//				int distance = 0;

//				for (int j = 0; j < binaryHandRotated.rows; j++) {
//					const bool val = binaryHandRotated.at<uint8_t>(i, j) > 0;
//					if (edge1 == -1) {
//						if (val) {
//							edge1 = j;
//						}
//					} else {
//						if (edge2 == -1) {
//							if (!val) {
//								edge2 = j;
//							}
//						} else {
//							if (val) {
//								if (edge2 - edge1 > distance) {
//									distance = edge2 - edge1;
//									palmLine.first = cv::Point(edge1, i);
//									palmLine.second = cv::Point(edge2, i);
//								}
//							}
//						}
//					}
//				}

//				// must have found something
//				if (distance > 0) {

//				}
//			}
//			WindowManager::getInstance().imShow("rotated", binaryHandRotated);

//		}
*/
        /// ~~~~
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100), 1);
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius*PALM_RADIUS_RATIO, palmRadius*PALM_RADIUS_RATIO), 0, 0, 360, cv::Scalar(180), 1);
	cv::putText(fingerMaskOutput, QString::number(handContour.size()).toStdString(), cv::Point(10, 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
        cv::putText(fingerMaskOutput, QString::number(fingersContours.size()).toStdString(), cv::Point(10, 22), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
//        cv::putText(fingerMaskOutput, QString::number(kkk).toStdString(), cv::Point(10, 35), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));

	/// Draw

	cv::Mat centerHighlited = bgr.clone();

	wristpair_t wrist = data.wrist();
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

void HandTracker::C1::setVector(QVector<float> *vector)
{
	m_vector = vector;
}

void HandTracker::Data::setFingertips(QList<cv::Point> fingertips)
{
	QMutexLocker l(&m_mutex);
	m_fingertips = fingertips;
}

QList<cv::Point> HandTracker::Data::fingertips() const
{
	QMutexLocker l(&m_mutex);
	return m_fingertips;
}

PoseRecognition *HandTracker::Data::pose()
{
	return &m_pose;
}

} // Namespace iez
