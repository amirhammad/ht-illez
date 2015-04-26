#include "HandTracker.h"
#include "Processing.h"

#include <vector>
#include <limits>
#include <queue>

namespace iez {

#define HAND_MAX_PHYSICAL_DEPTH	(150)
#define PALM_RADIUS_RATIO (1.7f)

// FINGER_FACTOR*PALM_RADIUS => minimal length of finger -- set to half the size of finger
#define FINGER_LENGTH_FACTOR (0.5f)
#define FINGER_MAXWIDTH_FACTOR (0.57f)

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

float HandTracker::findHandCenterRadius(const cv::Point& maxDTPoint, const std::vector<cv::Point> contour)
{
	float minDistance = std::numeric_limits<float>::max();

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

	for (float currentAngle = 0; currentAngle < 360.0f; currentAngle += 360.0f/maxValues) {
		cv::Point randomPoint;
		randomPoint.x = palmRadius*PALM_RADIUS_RATIO*cosf(currentAngle*2*M_PI/360.f);
		randomPoint.y = palmRadius*PALM_RADIUS_RATIO*sinf(currentAngle*2*M_PI/360.f);
		randomPoint += palmCenter;

		if ((randomPoint.y < 0 && randomPoint.y >= binaryHand.rows)
		&& (randomPoint.x < 0 && randomPoint.x >= binaryHand.cols)) {
			continue;
		}
		cv::Point nearestPoint = Processing::findNearestPoint(contour, randomPoint);
		if (!boundaryPointList.isEmpty()) {
			const float d = Processing::pointDistance(nearestPoint, boundaryPointList.last());
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

bool HandTracker::findWrist(const std::vector<cv::Point> &palmContour,
							const cv::Point &palmCenter,
							const float palmRadius,
							const Data &data,
							wristpair_t& outputWrist)
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
			Processing::pointDistance(palmContour[0], palmContour[palmContour.size() - 1]);
	for (int i = 0; i < palmContour.size() - 1; i++) {
		distanceToNeighbor[i] =
				Processing::pointDistance(palmContour[i], palmContour[i + 1]);
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

	wristpair_t wppData = data.wrist();
	cv::Point wppMeanData = Processing::pointMean(wppData.first, wppData.second);

	wristpair_t wppFinal = wppData;
	float minDist = std::numeric_limits<float>::max();
	wppFinal = wristpair_t(palmContour[wristPairCandidatesOrdered[0]], palmContour[(wristPairCandidatesOrdered[0] + 1)%palmContour.size()]);


	int cnt = 0;
	while (wristPairCandidatesOrdered.size() > 0 && cnt < 5) {
		wristpair_t wpp(palmContour[wristPairCandidatesOrdered[cnt]], palmContour[(wristPairCandidatesOrdered[cnt] + 1)%palmContour.size()]);
		cv::Point wppMean = Processing::pointMean(wpp.first, wpp.second);
		float d = Processing::pointDistance(wppMean, wppMeanData);

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
	wristPointPair = wristpair_t(Processing::pointMean(wppFinal.first, wppData.first, ratio),
							   Processing::pointMean(wppFinal.second, wppData.second, ratio));

	/// return wristpair
	if (0) {
		outputWrist = wristPointPair;
	} else {
		outputWrist = wristPairFix(palmCenter, palmRadius, Processing::pointMean(wristPointPair.first, wristPointPair.second));
	}
	return true;
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
	qSort(rectPointsVector.begin(), rectPointsVector.end(), compare1(palmCenter));


	int fingerCount = rectWidth/(palmRadius*FINGER_MAXWIDTH_FACTOR) + 1;
	if (fingerCount == 0) {
		return QList<cv::Point>();
	} else if (fingerCount > 1) {

		// add factor, because when fingers are close to each other, they appear thinner
		const float factor = 0.75f;
		fingerCount = rectWidth/(palmRadius*FINGER_MAXWIDTH_FACTOR*factor);

	}
	QList<cv::Point> l;
	Q_ASSERT(fingerCount > 0);

	if (fingerCount == 1) {
		cv::Point fingertip = Processing::pointMean(rectPointsVector[2], rectPointsVector[3]);

		l.append(fingertip);
	} else if (fingerCount < 5) {
		for (int i = 0; i < fingerCount; i++) {
			const float ratio = (i + 0.5f)/fingerCount;
			cv::Point fingertip = Processing::pointMean(rectPointsVector[2], rectPointsVector[3], ratio);
			l.append(fingertip);
		}
	} else {
		return QList<cv::Point>();
	}

	return l;
}

wristpair_t HandTracker::wristPairFix(cv::Point palmCenter, float palmRadius, cv::Point wristMiddle)
{
	const float distToCenter = Processing::pointDistance(palmCenter, wristMiddle);
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

HandTracker::Data HandTracker::data() const
{
	return m_data;
}
void HandTracker::findFingers(cv::Mat &binaryFingersMask,
							  std::vector<std::vector<cv::Point> > &fingersContours,
							  const cv::Mat &binaryHand,
							  const cv::Mat &palmMask,
							  const Data &data)
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
}

void HandTracker::invokeProcess(const cv::Mat &bgr, const cv::Mat &depth, const int imageId)
{
	process(bgr, depth, imageId, m_data);
}

HandTracker::~HandTracker()
{
}

void HandTracker::process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId, Data &data)
{
	QTime t;
	t.start();
	/// Create binary hand

	cv::Mat binaryHand;
	std::vector<cv::Point> handContour;
	cv::Mat candidates;

	cv::Point nearestPoint; // used for selection of hand from multiple components (nearest)

	int near = Processing::findMin2(depth, nearestPoint);
	int far = near + HAND_MAX_PHYSICAL_DEPTH;

	candidates = Processing::filterDepthMask(depth, near, far);

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
		return;
	} else if (contours.size() == 1) {
		handContour = contours[0];
	} else {
		float minDist = std::numeric_limits<float>::max();
		int minIndex = -1;
		cv::Point mid;
		for (int i = 0; i < contours.size(); i++) {
			const cv::Point tmpmid = Processing::calculateMean(contours[i]);
			const float d = Processing::pointDistance(tmpmid, nearestPoint);
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

	cv::ellipse(out, nearestPoint, cv::Size(5, 5), 0, 0, 360, cv::Scalar_<uint8_t>(200), 2);
//	WindowManager::getInstance()->imShow("binaryHand", binaryHand);



	/// Distance Transform

	cv::Mat handDT;
	distanceTransform(binaryHand, handDT);

	/// Find hand center

	cv::Point palmCenter;
	findHandCenter(handDT, palmCenter);


	/// Find palm radius

	const float palmRadius = findHandCenterRadius(palmCenter, handContour);
	Q_ASSERT(palmRadius > 0);

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
	wristpair_t wrist;
	if (findWrist(palmContour, palmCenter, palmRadius, data, wrist) == false) {
		qDebug("ERROR %s %d", __FILE__, __LINE__);
		return;
	}

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
		data.setFingertips(fingertips);
		data.setPalmCenter(palmCenter);
		data.setPalmRadius(palmRadius);
		data.setWrist(wrist);
	}
	///
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100), 1);
	cv::ellipse(fingerMaskOutput, palmCenter, cv::Size(palmRadius*PALM_RADIUS_RATIO, palmRadius*PALM_RADIUS_RATIO), 0, 0, 360, cv::Scalar(180), 1);
	cv::putText(fingerMaskOutput, QString::number(handContour.size()).toStdString(), cv::Point(10, 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
	cv::putText(fingerMaskOutput, QString::number(fingersContours.size()).toStdString(), cv::Point(10, 22), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));
//        cv::putText(fingerMaskOutput, QString::number(kkk).toStdString(), cv::Point(10, 35), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255));

	/// Draw

	cv::Mat centerHighlited = bgr.clone();

	cv::line(centerHighlited, wrist.first, wrist.second, cv::Scalar(100), 5);
	cv::ellipse(centerHighlited, palmCenter, cv::Size(palmRadius, palmRadius), 0, 0, 360, cv::Scalar(100, 0, 255), 10);


	WindowManager::getInstance()->imShow("handCenter", centerHighlited);

	// draw only if valid, else results in flickering...
	if (fingerMaskOutput.rows == binaryHand.rows) {
		WindowManager::getInstance()->imShow("fingers", fingerMaskOutput);
	}
//	WindowManager::getInstance()->imShow("PALM", palmMask);

	qDebug("ID=%5d fps: %4.1f ... %3dms", imageId, (1000/30.f)/t.elapsed()*30, t.elapsed());
}

void HandTracker::orderFingertipsByAngle(wristpair_t wrist, QList<cv::Point> &fingertips)
{
	const cv::Point2f wristMiddlePoint = Processing::pointMean(wrist.first, wrist.second);
	// sort by angle
	struct compare {
		compare(cv::Point palmCenter, float offs)
		:	m_palmCenter(palmCenter)
		,	m_offs(offs) {

		}

		float getAngle(cv::Point point) {
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
