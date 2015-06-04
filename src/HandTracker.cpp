/*
 * This file is part of the project HandTrackerApp - ht-illez
 *
 * Copyright (C) 2015 Amir Hammad <amir.hammad@hotmail.com>
 *
 *
 * HandTrackerApp - ht-illez is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "HandTracker.h"
#include "Util.h"
#include "WindowManager.h"
#include "Settings.h"

#include <vector>
#include <limits>
#include <QTime>

namespace iez {

#define HAND_MAX_PHYSICAL_DEPTH	(150)
#define PALM_RADIUS_RATIO (1.7f)

// FINGER_FACTOR*PALM_RADIUS => minimal length of finger -- set to half the size of finger
#define FINGER_LENGTH_FACTOR (0.5f)
#define FINGER_MAXWIDTH_FACTOR (0.57f)


/**
 * @brief HandTracker::distanceTransform - distance transformation on binary image, uses Euclidean norm
 * @param binaryHandFiltered[in] binary image
 * @param handDT[out] distance transform matrix
 */
void HandTracker::distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT) const
{
	cv::distanceTransform(binaryHandFiltered, handDT, CV_DIST_L2, 3);
}

/**
 * @brief HandTracker::findHandCenter
 * @param handDT[in] matrix containing distance transform information
 * @param maxDTPoint[out]
 */
void HandTracker::findHandCenter(const cv::Mat &handDT, cv::Point &maxDTPoint) const
{
	Q_UNUSED(Util::findMax2(handDT, maxDTPoint));
}

/**
 * @brief HandTracker::findHandCenterRadius
 * @param maxDTPoint[in] hand center point
 * @param contour[in] contour of hand
 * @return minimal distance to point on contour
 */
float HandTracker::findHandCenterRadius(const cv::Point& maxDTPoint, const std::vector<cv::Point> &contour) const
{
	float minDistance = std::numeric_limits<float>::max();

	for (int i = 0; i < contour.size(); i++) {
		float dist = Util::pointDistance(maxDTPoint, contour[i]);
		if ( dist < minDistance) {
			minDistance = dist;
		}
	}
	return minDistance;
}


/**
 * @brief HandTracker::findPalm
 * @param binaryPalmMask[out]
 * @param palmContour[out]
 * @param binaryHand[in]
 * @param contour[in]  hand contour
 * @param palmCenter[in]
 * @param palmRadius[in]
 */
void HandTracker::findPalm(cv::Mat &binaryPalmMask,
						   std::vector<cv::Point> &palmContour,
						   const cv::Mat &binaryHand,
						   const std::vector<cv::Point> &contour,
						   const cv::Point &palmCenter,
						   const float palmRadius) const
{
	const int maxValues = 100;


	QList<cv::Point> boundaryPointList;
	boundaryPointList.reserve(maxValues);

	for (float currentAngle = 0; currentAngle < 360.0f; currentAngle += 360.0f/maxValues) {
		cv::Point randomPoint;
		randomPoint.x = palmRadius*PALM_RADIUS_RATIO*cosf(currentAngle*2*M_PI/360.f);
		randomPoint.y = palmRadius*PALM_RADIUS_RATIO*sinf(currentAngle*2*M_PI/360.f);
		randomPoint += palmCenter;

		if ((randomPoint.y < 0 && randomPoint.y >= binaryHand.rows)
		&& (randomPoint.x < 0 && randomPoint.x >= binaryHand.cols)) {
			continue;
		}
		cv::Point nearestPoint = Util::findNearestPoint(contour, randomPoint);
		if (!boundaryPointList.isEmpty()) {
			const float d = Util::pointDistance(nearestPoint, boundaryPointList.last());
			if (d < 10.0f) {
				// Relax
			} else {
				boundaryPointList.append(nearestPoint);
			}
		} else {
			boundaryPointList.append(nearestPoint);
		}

	}

	/// SORT by angle

	struct compare {
		compare(cv::Point palmCenter)
		:	m_palmCenter(palmCenter){

		}

		float getAngle(cv::Point point) const {
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
	qSort(boundaryPointList.begin(), boundaryPointList.end(), compare(palmCenter));


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

/**
 * @brief HandTracker::findWrist
 * @param palmContour[in]
 * @param palmCenter[in]
 * @param palmRadius[in]
 * @param outputWrist[out]
 * @return
 */
bool HandTracker::findWrist(const std::vector<cv::Point> &palmContour,
							const cv::Point &palmCenter,
							const float palmRadius,
							wristpair_t& outputWrist) const
{
	wristpair_t wristPointPair;

	if (palmContour.size() <= 2) {
		qDebug("Error: palmContour.size() <= 2");
		return false;
	}

	/**
	 * construct the Priority queue and take out first 5 for consideration.
	 * Then choose closest one to previous data.wrist (if valid).
	 */

	/// calculate distances between neighbors
	QVector<float> distanceToNeighbor(palmContour.size());
	distanceToNeighbor[distanceToNeighbor.size() - 1] =
			Util::pointDistance(palmContour[0], palmContour[palmContour.size() - 1]);
	for (int i = 0; i < palmContour.size() - 1; i++) {
		distanceToNeighbor[i] =
				Util::pointDistance(palmContour[i], palmContour[i + 1]);
	}


	/// comparator for PQ
	// Comparator for findWrist
	struct C1 {
	public:
		explicit C1(QVector<float> vector):m_vector(vector){}
	private:
		QVector<float> m_vector;
	public:
		/// greater
		bool operator()(int a, int b) const {return m_vector[a] > m_vector[b]; }
	};

	QList<int> wristPairCandidatesOrdered;
	wristPairCandidatesOrdered.reserve(distanceToNeighbor.size());

	for (int i = 0; i < distanceToNeighbor.size(); i++) wristPairCandidatesOrdered.append(i);

	qSort(wristPairCandidatesOrdered.begin(), wristPairCandidatesOrdered.end(), C1(distanceToNeighbor));

	/// Find wrist(out of 5 got in previous algorithm step) nearest to previously saved wrist

	wristpair_t wppData = m_data.wrist();
	cv::Point wppMeanData = Util::pointMean(wppData.first, wppData.second);

	wristpair_t wppFinal;
	float minDist = std::numeric_limits<float>::max();
	wppFinal = wristpair_t(palmContour[wristPairCandidatesOrdered[0]], palmContour[(wristPairCandidatesOrdered[0] + 1)%palmContour.size()]);


	int cnt = 0;
	while (wristPairCandidatesOrdered.size() > 0 && cnt < 5) {
		wristpair_t wpp(palmContour[wristPairCandidatesOrdered[cnt]], palmContour[(wristPairCandidatesOrdered[cnt] + 1)%palmContour.size()]);
		cv::Point wppMean = Util::pointMean(wpp.first, wpp.second);
		float d = Util::pointDistance(wppMean, wppMeanData);

		//using physical depth instead of ~size of hand... it is equal..
		if (minDist > d && ( wppMeanData == cv::Point(0, 0) || d < HAND_MAX_PHYSICAL_DEPTH)) {
			minDist = d;
			wppFinal = wpp;
		}

		cnt++;
	}

	/// order pair by right-hand rule

	const cv::Point &pt1 = wppFinal.first;
	const cv::Point &pt2 = palmCenter;
	const cv::Point &pt3 = wppFinal.second;
	const cv::Point normal(pt2.y - pt1.y, pt1.x - pt2.x); // (from pt1 to pt2) -> normal -90*
	if ((pt3 - pt1).dot(normal) < 0) {
		// swap
		wppFinal = wristpair_t(wppFinal.second, wppFinal.first);
	}

	/// lowpassing
	const float ratio = 0.8f;
	wristPointPair = wristpair_t(Util::pointMean(wppFinal.first, wppData.first, ratio),
							   Util::pointMean(wppFinal.second, wppData.second, ratio));

	/// return wristpair
	outputWrist = wristPairFix(palmCenter, palmRadius, Util::pointMean(wristPointPair.first, wristPointPair.second));

	return true;
}

/**
 * @brief HandTracker::findFingertip returns one or more fingertips contained in bounding rectangle defined by rotRect
 * @param rotRect[in]
 * @param palmRadius[in]
 * @param palmCenter[in]
 * @return list of fingertips contained in the current rectangle
 */
QList<cv::Point> HandTracker::findFingertip(const cv::RotatedRect &rotRect,
											const float palmRadius,
											const cv::Point &palmCenter) const
{
	cv::Point2f rectPoints[4];
	rotRect.points(rectPoints);

	const float rectWidth = std::min(rotRect.size.width, rotRect.size.height);
	// find fingertips
	struct compare1 {
		compare1(cv::Point palmCenter)
		:	m_palmCenter(palmCenter){

		}

		int operator()(cv::Point2f p1, cv::Point2f p2) {
			float p1d = Util::pointDistance(p1, m_palmCenter);
			float p2d = Util::pointDistance(p2, m_palmCenter);
			return p1d < p2d;
		}
		cv::Point m_palmCenter;
	};

	std::vector<cv::Point2f> rectPointsVector(4);
	for (int i = 0; i < 4; i++) {
		rectPointsVector[i] = rectPoints[i];
	}
	qSort(rectPointsVector.begin(), rectPointsVector.end(), compare1(palmCenter));


	int fingerCount = rectWidth/(palmRadius*FINGER_MAXWIDTH_FACTOR) + 1;
	if (fingerCount > 1) {

		// add factor, because when fingers are close to each other, they appear thinner
		const float factor = 0.75f;
		fingerCount = rectWidth/(palmRadius*FINGER_MAXWIDTH_FACTOR*factor);

	}
	QList<cv::Point> l;
	Q_ASSERT(fingerCount > 0);

	if (fingerCount == 1) {
		cv::Point fingertip = Util::pointMean(rectPointsVector[2], rectPointsVector[3]);

		l.append(fingertip);
	} else if (fingerCount < 5) {
		for (int i = 0; i < fingerCount; i++) {
			const float ratio = (i + 0.5f)/fingerCount;
			cv::Point fingertip = Util::pointMean(rectPointsVector[2], rectPointsVector[3], ratio);
			l.append(fingertip);
		}
	} else {
		return QList<cv::Point>();
	}

	return l;
}
/**
 * @brief HandTracker::wristPairFix
 * @param palmCenter[in]
 * @param palmRadius[in]
 * @param wristMiddle[in]
 * @return fixed wrist
 *
 * Rotates and scales wrist based on wrists' middle point, palmRadius, palmCenter
 * Line created by wristpair points is perpendicular to the line between palmCenter and wristMiddle.
 * This line has wristMiddle in the middle and has length of palmRadius
 *
 */
wristpair_t HandTracker::wristPairFix(cv::Point palmCenter, float palmRadius, cv::Point wristMiddle) const
{
	const float distToCenter = Util::pointDistance(palmCenter, wristMiddle);
	cv::Point2f vecToCenter = (palmCenter - wristMiddle);
	// normalize
	vecToCenter.x /= distToCenter;
	vecToCenter.y /= distToCenter;

	// create normal vector
	cv::Point2f normalVector = cv::Point2f(vecToCenter.y, -vecToCenter.x);


	// ASSUMPTION: wrist width is equal to palmRadius
	wristpair_t wrist;
	wrist.first = cv::Point2f(wristMiddle.x, wristMiddle.y) - 0.5f*palmRadius*normalVector;
	wrist.second = cv::Point2f(wristMiddle.x, wristMiddle.y) + 0.5f*palmRadius*normalVector;
	return wrist;
}

// not thread-safe
HandTracker::Data HandTracker::data() const
{
	return m_data;
}

// not thread-safe
HandTracker::TemporaryResult HandTracker::temporaryResult() const
{
	return m_temp;
}

void HandTracker::imshow(QString name, cv::Mat mat)
{
	WindowManager::getInstance()->imShow(name, mat);
}

void HandTracker::findFingers(cv::Mat &binaryFingersMask,
							  std::vector<std::vector<cv::Point> > &fingersContours,
							  const cv::Mat &binaryHand,
							  const cv::Mat &palmMask) const
{
	cv::Mat tmp;
	cv::dilate(palmMask, tmp, cv::Mat(), cv::Point(-1,-1), 2);// dilate main
	cv::Mat binaryFingersDiff = binaryHand - tmp;
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(binaryFingersDiff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	/// use dot product with normal to remove contures under wrist line (half-plane filter out)

	wristpair_t wrist = m_data.wrist();
	QList<std::vector<cv::Point> > l;
	if (isDebug()) {
		m_temp.fingerContoursIgnoredList.clear();
	}
	for (int i = 0; i < contours.size(); i++) {
		cv::Point center = Util::calculateMean(contours[i]);
		const cv::Point &pt1 = wrist.first;
		const cv::Point &pt2 = wrist.second;
		const cv::Point &pt3 = center;
		const cv::Point normal(pt2.y - pt1.y, pt1.x - pt2.x); // (from pt1 to pt2) -> normal -90*
		if ((pt3 - pt1).dot(normal) < 0) {
			l.append(contours[i]);
		} else {
			m_temp.fingerContoursIgnoredList.append(contours[i]);
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
	: m_lastImageId(-100)
{
	m_bDebug = Settings::instance()->value("HandTracker::m_bDebug").toBool();
}

HandTracker::~HandTracker()
{
}

// not thread-safe
void HandTracker::process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId)
{
	if (imageId <= m_lastImageId) {
		return;
	} else {
		m_lastImageId = imageId;
	}
	QTime t;
	t.start();
	/// Create binary hand

	cv::Mat binaryHand;
	std::vector<cv::Point> handContour;
	cv::Mat candidates;

	cv::Point nearestPoint; // used for selection of hand from multiple components (nearest)

	int near = Util::findMin2(depth, nearestPoint);
	int far = near + HAND_MAX_PHYSICAL_DEPTH;

	candidates = Util::filterDepthMask(depth, near, far);

	/// Filter
	cv::Mat candidatesFiltered;
	cv::medianBlur(candidates, candidatesFiltered, 5);

	if (isDebug()) {
		m_temp.medianList.clear();
		for (uint i = 1; i <= 9; i += 2) {
			cv::Mat tmp;
			cv::medianBlur(candidates, tmp, i);
			cv::putText(tmp, QString::number(i).toStdString(), cv::Point(10, 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
			m_temp.medianList.append(tmp);
		}

		m_temp.originalColor = bgr;
		m_temp.originalDepth = depth;
		cv::Mat l;
		bgr.copyTo(l, candidatesFiltered);
		m_temp.depthMaskedImage = l;
	}



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
		return;
	} else if (contours.size() == 1) {
		handContour = contours[0];
	} else {
		float minDist = std::numeric_limits<float>::max();
		int minIndex = -1;
		cv::Point mid;
		for (int i = 0; i < contours.size(); i++) {
			const cv::Point tmpmid = Util::calculateMean(contours[i]);
			const float d = Util::pointDistance(tmpmid, nearestPoint);
			if (contours[i].size() < 20) {
				continue;
			}
			if (minDist > d) {
				minDist = d;
				minIndex = i;
				mid = tmpmid;
			}
		}
		// not found valid contour
		if (minIndex == -1) {
			qDebug("%d Zero objects", imageId);
			return;
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

	/// save binary hand
	out.copyTo(binaryHand);
	if (isDebug()) {
		m_temp.handContour = handContour;
		binaryHand.copyTo(m_temp.handMask);
	}

	cv::ellipse(out, nearestPoint, cv::Size(5, 5), 0, 0, 360, cv::Scalar_<uint8_t>(200), 2);
//	imshow("binaryHand", binaryHand);



	/// Distance Transform

	cv::Mat handDT;
	distanceTransform(binaryHand, handDT);
	if (isDebug()) {
		m_temp.distanceTransform = handDT;
	}

	/// Find hand center

	cv::Point palmCenter;
	findHandCenter(handDT, palmCenter);
	if (isDebug()) {
		m_temp.palmCenter = palmCenter;
	}


	/// Find palm radius

	const float palmRadius = findHandCenterRadius(palmCenter, handContour);

	if (isDebug()) {
		m_temp.palmRadius = palmRadius;
	}

	Q_ASSERT(palmRadius > 0);

	/// ready: handCenter, innerCircleRadius, binaryHand(filtered), handContour

	/// find palm mask
	cv::Mat palmMask;
	std::vector<cv::Point> palmContour;
	findPalm(palmMask, palmContour, binaryHand, handContour, palmCenter, palmRadius);
	if (isDebug()) {
		m_temp.palmMask = palmMask;
		m_temp.palmContour = palmContour;
	}

	// ignore small palmContours
	float area = cv::contourArea(palmContour);
	if (area < 1000) {
		qDebug("contourAreaFail %f", area);
		return;
	}

	/// isolate fingers, ready: +palmMask

	/// find wrist
	wristpair_t wrist;
	if (findWrist(palmContour, palmCenter, palmRadius, wrist) == false) {
		qDebug("ERROR %s %d", __FILE__, __LINE__);
		return;
	}

	/// find fingers

	cv::Mat fingersMask;
	std::vector<std::vector<cv::Point> > fingersContours;
	findFingers(fingersMask, fingersContours, binaryHand, palmMask);

	if (isDebug()) {
		m_temp.fingersContours = fingersContours;
		m_temp.fingersMask = fingersMask;
	}



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
		for (int j = 0; j < 4; j++) {
			cv::line(fingerMaskOutput, rect_points[j], rect_points[(j+1)%4], cv::Scalar(100), 2, 8 );
		}

		{
			// unmerge merged fingertips
			QList<cv::Point> rectFingertips = findFingertip(rotRect, palmRadius, palmCenter);
			foreach (cv::Point fingertip, rectFingertips) {
				fingertips.append(fingertip);
//				cv::ellipse(fingerMaskOutput, fingertip, cv::Size(2, 2), 0, 0, 360, cv::Scalar(100), 5);
			}
		}
	}

	if (isDebug()) {
		m_temp.fingertipsNonSorted = fingertips;
	}

	orderFingertipsByAngle(wrist, fingertips);

	// draw fingertips
	{
		int i = 0;
		foreach (cv::Point fingertip, fingertips) {
			cv::ellipse(fingerMaskOutput, fingertip, cv::Size(2, 2), 0, 0, 360, cv::Scalar(100), 5);
			cv::putText(fingerMaskOutput, QString::number(i++).toStdString(), fingertip, cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
		}
	}

	{	// FINALIZE
		m_data.setFingertips(fingertips);
		m_data.setPalmCenter(palmCenter);
		m_data.setPalmRadius(palmRadius);
		m_data.setWrist(wrist);
	}
	///
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100), 1);
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius*PALM_RADIUS_RATIO, palmRadius*PALM_RADIUS_RATIO), 0, 0, 360, cv::Scalar(180), 1);
//	cv::putText(fingerMaskOutput, QString::number(handContour.size()).toStdString(), cv::Point(10, 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
//	cv::putText(fingerMaskOutput, QString::number(fingersContours.size()).toStdString(), cv::Point(10, 22), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
//        cv::putText(fingerMaskOutput, QString::number(kkk).toStdString(), cv::Point(10, 35), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));

	/// Draw

	cv::Mat centerHighlited = bgr.clone();

	cv::line(centerHighlited, wrist.first, wrist.second, cv::Scalar(100), 5);
	cv::ellipse(centerHighlited, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100, 0, 255), 10);


	imshow("handCenter", centerHighlited);

	// draw only if valid, else results in flickering...
	if (fingerMaskOutput.rows == binaryHand.rows) {
		imshow("fingers", fingerMaskOutput);
		if (isDebug()) {
			m_temp.result = fingerMaskOutput;
		}
	}
//	imshow("PALM", palmMask);

	qDebug("ID=%5d fps: %4.1f ... %3dms", imageId, (1000/30.f)/t.elapsed()*30, t.elapsed());
}

/**
 * @brief HandTracker::orderFingertipsByAngle
 * @param wrist[in]
 * @param fingertips[in/out]
 */
void HandTracker::orderFingertipsByAngle(wristpair_t wrist, QList<cv::Point> &fingertips)
{
	const cv::Point2f wristMiddlePoint = Util::pointMean(wrist.first, wrist.second);
	// sort by angle
	struct compare {
		compare(cv::Point palmCenter, float offs)
		:	m_palmCenter(palmCenter)
		,	m_offs(offs) {

		}

		float getAngle(cv::Point point) const {
			const float dy = point.y - m_palmCenter.y;
			const float dx = point.x - m_palmCenter.x;
			const float angle = atan2f(dy, dx) - (m_offs);
			if (angle < -M_PI) {
				return angle + 2*M_PI;
			} else {
				return angle;
			}
		}

		bool operator() (cv::Point a, cv::Point b) {
			return getAngle(a) < getAngle(b);
		}

		const cv::Point m_palmCenter;
		const float m_offs;
	};

	const float offs = atan2f(wrist.second.y - wrist.first.y,
						wrist.second.x - wrist.first.x);

	qSort(fingertips.begin(), fingertips.end(), compare(cv::Point(wristMiddlePoint.x, wristMiddlePoint.y), offs));
}


void HandTracker::Data::setWrist(std::pair<cv::Point, cv::Point> wrist)
{
	m_wrist = wrist;
}

std::pair<cv::Point, cv::Point> HandTracker::Data::wrist() const
{
	return m_wrist;
}

void HandTracker::Data::setFingertips(QList<cv::Point> fingertips)
{
	m_fingertips = fingertips;
}

QList<cv::Point> HandTracker::Data::fingertips() const
{
	return m_fingertips;
}
float HandTracker::Data::palmRadius() const
{
	return m_palmRadius;
}

void HandTracker::Data::setPalmRadius(float palmRadius)
{
	m_palmRadius = palmRadius;
}

cv::Point HandTracker::Data::palmCenter() const
{
	return m_palmCenter;
}

void HandTracker::Data::setPalmCenter(const cv::Point &palmCenter)
{
	m_palmCenter = palmCenter;
}


} // Namespace iez
