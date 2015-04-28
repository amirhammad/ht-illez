#include "Processing.h"
#include "ImageSource.h"
#include <stdint.h>
#include <string>
#include <math.h>
//#include <QtGui>
//#include <QtCore>
//#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include "main.h"
#include <QApplication>

//using namespace cv;
namespace iez {

Processing::Processing(ImageSourceBase *imgsrc, QObject *parent)
:	QObject(parent)
,	m_imageSource(imgsrc)
,	m_calculateHandTracker(false)
,	m_handTracker(true)
{
	m_thread = new QThread(QCoreApplication::instance()->thread());
	moveToThread(m_thread);

	connect(imgsrc, SIGNAL(frameReceived()), this, SLOT(process()));
	connect(this, SIGNAL(got_learnNew(int)), this, SLOT(on_learnNew(int)));
	connect(this, SIGNAL(got_train()), this, SLOT(on_train()));
	m_thread->start();
}


Processing::~Processing(void)
{
	qDebug("X1");

	m_thread->exit();
	// If thread does not terminate within 10s, terminate it
	if (!m_thread->wait(10000)) {
		m_thread->terminate();
		m_thread->wait();
	}
	m_thread->deleteLater();

	qDebug("X1 finished");
}


void Processing::process(bool secondarySource)
{
	QMutexLocker l(&m_processMutex);
	const ImageSourceBase *src;
	if (secondarySource) {
		if (m_secondarySource) {
			src = m_secondarySource.data();
		} else {
			Q_ASSERT("secondary source not initialized");
		}
	} else {
		src = m_imageSource;
	}
	const cv::Mat &depth = m_imageSource->getDepthMat();
	const cv::Mat &rgb = m_imageSource->getColorMat();
	cv::Mat bgr;
	cv::cvtColor(rgb, bgr,cv::COLOR_RGB2BGR);

	Q_ASSERT(bgr.rows == depth.rows);
	Q_ASSERT(bgr.cols == depth.cols);

//	WindowManager::getInstance()->imShow("Original", bgr);

	m_handTracker.process(bgr, depth, m_imageSource->getSequence());
	const HandTracker::Data handTrackerData = m_handTracker.data();

	QString poseString = m_pose.categorize(handTrackerData.palmCenter(),
					  handTrackerData.palmRadius(),
					  handTrackerData.wrist(),
					  handTrackerData.fingertips());
	emit got_poseUpdated(poseString);
	imageSourceArtificial->setColorMat(bgr);

	if (secondarySource) {
		m_handTrackerTemporaryResult = m_handTracker.temporaryResult();
	}
	m_handTrackerResult = m_handTracker.data();
}

void Processing::learnNew(enum PoseRecognition::POSE poseId)
{
	emit got_learnNew(poseId);
}

void Processing::on_learnNew(int poseId)
{
	const HandTracker::Data data = m_handTracker.data();
	qDebug("LEARNING");
	qDebug("%d", poseId);
	qDebug("(%d, %d) R=%f", data.palmCenter().x, data.palmCenter().y, data.palmRadius());
	qDebug("(%d, %d) (%d, %d)", data.wrist().first.x,
								data.wrist().first.y,
								data.wrist().second.x,
								data.wrist().second.y);
	qDebug("fingertips:");
	foreach (cv::Point p, data.fingertips()) {
		qDebug("\t(%d, %d)", p.x, p.y);
	}

	m_pose.learnNew(static_cast<enum PoseRecognition::POSE>(poseId),
				  data.palmCenter(),
				  data.palmRadius(),
				  data.wrist(),
					data.fingertips());
}

void Processing::train()
{
	emit got_train();
}

void Processing::on_train()
{
	try {
		m_pose.train();
	} catch (std::logic_error e) {
		qDebug("%s", e.what());
		QApplication::quit();
	}

	emit got_trainingFinished();
}

int Processing::findMin(const cv::Mat& depth)
{
	cv::Point dummy;
	return findMin2(depth, dummy);
}

int Processing::findMin2(const cv::Mat& depth, cv::Point &point)
{
	int minDepth = 10000;
	for (int i = 0; i < depth.rows; i++) {
		for (int j = 0; j < depth.cols; j++) {
			const int d = depth.at<uint16_t>(i, j);
			if (d > 0) {
				if (minDepth > d) {
					minDepth = d;
					point = cv::Point(j, i);
				}
			}
		}
	}

	return minDepth;
}


void Processing::filterDepth(cv::Mat &dst, const cv::Mat &depth, int near, int far)
{
	Q_ASSERT(dst.rows == depth.rows);
	Q_ASSERT(dst.cols == depth.cols);

	if (near == -1) {
		near = findMin(depth);
		far = near + 170;
	}
	const cv::Mat &mask = filterDepthMask(depth, near, far);
	WindowManager::getInstance()->imShow("x", mask);
	cv::Mat dst2;

	dst.copyTo(dst2, mask);
	WindowManager::getInstance()->imShow("x2", dst2);
	dst2.copyTo(dst);
}

cv::Mat Processing::filterDepthMask(const cv::Mat &depth, int near, int far)
{
	cv::Mat outputMask;
	outputMask.create(depth.rows, depth.cols, CV_8UC1);

	if (near == -1) {
		near = findMin(depth);
		far = near + 150;
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

	for (int i = 0; i < pointVector.size(); i++) {
		mean = mean + pointVector[i];
	}
	int size = pointVector.size();
	return cv::Point(mean.x/size, mean.y/size);
}

cv::Point Processing::findNearestPoint(const std::vector<cv::Point> &pointVector, const cv::Point refPoint)
{
	float minDistance = std::numeric_limits<float>::max();
	if (pointVector.size() == 0) {
		return cv::Point();
	}
	cv::Point nearestPoint = pointVector[0];
	for (int i = 1; i < pointVector.size(); i++) {
		float d = pointDistance(refPoint, pointVector[i]);
		if (minDistance > d) {
			minDistance = d;
			nearestPoint = pointVector[i];
		}
	}
	return nearestPoint;
}

/**
 * Rotate an image
 */
void Processing::rotate(cv::Mat& src, double angle, cv::Mat& dst)
{
	int len = std::max(src.cols, src.rows);
	cv::Point2f pt(len/2., len/2.);
	cv::Mat r = cv::getRotationMatrix2D(pt, angle, 1.0);

	cv::warpAffine(src, dst, r, cv::Size(len, len), cv::INTER_NEAREST);
}

PoseRecognition *Processing::pose()
{
	return &m_pose;
}

void Processing::setSecondarySource(ImageSourceBase *secondarySource)
{
	m_secondarySource = secondarySource;
}

HandTracker::TemporaryResult Processing::handTrackerTemporaryResult() const
{
	return m_handTrackerTemporaryResult;
}

HandTracker::Data Processing::handTrackerData() const
{
	return m_handTrackerResult;
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

float Processing::pointDistance(const cv::Point& pt1, const cv::Point& pt2)
{
	const cv::Point diff = pt1 - pt2;
	return sqrtf(diff.dot(diff));
}

cv::Point Processing::pointMean(const cv::Point &pt1, const cv::Point &pt2, const float ratio12)
{
	return cv::Point((pt1.x*ratio12 + pt2.x*(1 - ratio12)), (pt1.y*ratio12 + pt2.y*(1 - ratio12)));
}

}
