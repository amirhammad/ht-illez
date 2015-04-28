#pragma once

#include <opencv2/opencv.hpp>
#include <list>
#include <string>
#include "main.h"
#include "ImageSourceFreenect.h"
#include "WindowManager.h"
#include "HandTracker.h"
#include <QPointer>
#include <QThread>

namespace iez {


class Processing : public QObject {
	Q_OBJECT
public:
	explicit Processing(ImageSourceBase *imgsrc, QObject *parent = 0);
	~Processing(void);

	void learnNew(enum PoseRecognition::POSE);
	void train();
	PoseRecognition *pose();
	void setSecondarySource(ImageSourceBase *secondarySource);
	HandTracker::TemporaryResult handTrackerTemporaryResult() const;
	HandTracker::Data handTrackerData() const;

	static float pointDistance(const cv::Point &pt1, const cv::Point &pt2);
	static cv::Point pointMean(const cv::Point &pt1, const cv::Point &pt2, const float ratio12 = 0.5f);
	static void filterDepth(cv::Mat &dst, const cv::Mat &src, int near = -1, int far = -1);
	static cv::Mat filterDepthMask(const cv::Mat &src, int near = -1, int far = -1);
	static int findMin(const cv::Mat &depth);
	static int findMin2(const cv::Mat &depth, cv::Point &point);
	static cv::Point calculateMean(const std::vector<cv::Point>&);
	static cv::Point findNearestPoint(const std::vector<cv::Point> &pointVector, const cv::Point refPoint);
	static void rotate(cv::Mat& src, double angle, cv::Mat& dst);

private:
	static cv::Point calculateWeightedMean(const std::vector<cv::Point>&);

	static cv::Point calculateMeanIndices(const cv::Mat&);

public slots:
	void process(bool secondarySource = false);

private slots:
	void on_learnNew(int poseId);
	void on_train();
private:
	QThread *m_thread;

	HandTracker m_handTracker;
	const ImageSourceBase *m_imageSource;
	QPointer<ImageSourceBase> m_secondarySource;
	bool m_calculateHandTracker;

	PoseRecognition m_pose;

	HandTracker::Data m_handTrackerResult;
	HandTracker::TemporaryResult m_handTrackerTemporaryResult;
	QMutex m_processMutex;
signals:
	void got_poseUpdated(QString);
	void got_learnNew(int);
	void got_train();
	void got_trainingFinished();
};

}




