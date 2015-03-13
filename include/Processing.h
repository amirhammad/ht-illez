#pragma once

#include <opencv2/opencv.hpp>
#include <list>
#include <string>
#include "main.h"
#include "ColorSegmentation.h"
#include "ImageSourceFreenect.h"
#include "WindowManager.h"
#include "HandTracker.h"

namespace iez {


class Processing : QObject {
	Q_OBJECT
public:
	explicit Processing(ImageSourceBase *imgsrc);
	~Processing(void);
	static cv::Mat processSaturate(const cv::Mat &bgr, const int satIncrease);
	static std::vector<cv::Point> smoothPoints(const std::vector<cv::Point> &vec, const int range);
	static float pointDistance(const cv::Point &pt1, const cv::Point &pt2);
	static void processHSVFilter(const cv::Mat &orig);
	static void filterDepth(cv::Mat &dst, const cv::Mat &src, int near = -1, int far = -1);
	static cv::Mat filterDepth2(const cv::Mat &src, int near = -1, int far = -1);
	static int findMin(const cv::Mat &depth);
private:


	static void processDepthFiltering(const cv::Mat &bgr, const cv::Mat &depth, cv::Mat &bgrDepthMasked, cv::Mat &bgrRoi, int near = -1);

	void processColorSegmentation(const cv::Mat &bgr, const cv::Mat &depth);

	void processContourTracing(const cv::Mat &bgr, const cv::Mat &depth, const cv::Mat &bgrDepthFiltered);
	static cv::Point calculateWeightedMean(const std::vector<cv::Point>&);
	static cv::Point calculateMean(const std::vector<cv::Point>&);
	static cv::Point calculateMeanIndices(const cv::Mat&);


	/// using K-means algorithm
//	static cv::Point calculateCentroids();
//	static std::list<cv::Point> calculate

	void processContourPoints(const cv::Mat &bgr, const cv::Mat &depth, const std::vector<cv::Point>& contour);
	std::vector<int> fingerCandidates(const std::vector<cv::Point>& contour, const std::vector<int> &hullIndices);
	std::vector<cv::Point> fingerCandidates2(const std::vector<cv::Point>& contour,
			const std::vector<int> &hullIndices,
			const cv::Mat &depth);
	std::vector<int> categorizeFingers(const std::vector<cv::Point>& contour ,const std::vector<int> &candidates);

public slots:
	void process();
private:
	QThread m_thread;

	struct Data {
		cv::Point center;
	} data;
	std::list<ImageStatistics> m_statsList;
	HandTracker m_handTracker;
	ImageSourceBase *m_imageSource;

	bool m_calculateHandTracker;
	ColorSegmentation *m_segmentation;
	Fps m_fps;


private slots:
	void keyPressEvent(QKeyEvent *keyEvent);
	void closeEvent();

};

}




