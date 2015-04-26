#pragma once

#include <opencv2/opencv.hpp>
#include <list>
#include <string>
#include "main.h"
#include "ColorSegmentation.h"
#include "ImageSourceFreenect.h"
#include "WindowManager.h"
#include "HandTracker.h"

class QSound;

namespace iez {


class Processing : public QObject {
	Q_OBJECT
public:
	explicit Processing(ImageSourceBase *imgsrc, QObject *parent = 0);
	~Processing(void);

	void learnNew(enum PoseRecognition::POSE);
	void train();

	static float pointDistance(const cv::Point &pt1, const cv::Point &pt2);
	static cv::Point pointMean(const cv::Point &pt1, const cv::Point &pt2, const float ratio12 = 0.5f);
	static void filterDepth(cv::Mat &dst, const cv::Mat &src, int near = -1, int far = -1);
	static cv::Mat filterDepthMask(const cv::Mat &src, int near = -1, int far = -1);
	static int findMin(const cv::Mat &depth);
	static int findMin2(const cv::Mat &depth, cv::Point &point);
	static cv::Point calculateMean(const std::vector<cv::Point>&);
	static cv::Point findNearestPoint(const std::vector<cv::Point> &pointVector, const cv::Point refPoint);
	static void rotate(cv::Mat& src, double angle, cv::Mat& dst);
	PoseRecognition *pose();

private:
	static cv::Point calculateWeightedMean(const std::vector<cv::Point>&);

	static cv::Point calculateMeanIndices(const cv::Mat&);

public slots:
	void process();
private slots:
	void on_learnNew(int poseId);
	void on_train();
private:
	QThread *m_thread;

	struct Data {
		cv::Point center;
	} data;
	std::list<ImageStatistics> m_statsList;
	HandTracker m_handTracker;
	const ImageSourceBase *m_imageSource;

	bool m_calculateHandTracker;
	const ColorSegmentation *m_segmentation;//not used
	Fps m_fps;

	PoseRecognition m_pose;

signals:
	void got_poseUpdated(QString);
	void got_learnNew(int);
	void got_train();
	void got_trainingFinished();
};

}




