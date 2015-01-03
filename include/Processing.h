#pragma once

#include <opencv2/opencv.hpp>
#include <list>
#include <string>
#include "ColorSegmentation.h"
#include "ImageSourceFreenect.h"
#include "WindowManager.h"

namespace iez {


class Processing : private QThread
{
	Q_OBJECT
public:
	explicit Processing(ImageSourceBase *imgsrc);
	~Processing(void);
	static cv::Mat processSaturate(const cv::Mat &bgr, const int satIncrease);

private:
	static void filterDepth(cv::Mat &dst, const cv::Mat &src, int near, int far);
	static void processDepthFiltering(const cv::Mat &bgr, const cv::Mat &depth, cv::Mat &bgrDepthMasked, cv::Mat &bgrRoi);

	void processColorSegmentation(const cv::Mat &bgr, const cv::Mat &depth);




	static void processHSVFilter(const cv::Mat &orig);
	void run();
	void process(const cv::Mat &bgr, const cv::Mat &depth);


	std::list<ImageStatistics> m_statsList;
//	CHandTracker m_handTracker;
	ImageSourceBase *const  m_imageSource;

	bool m_calculateHandTracker;
	ColorSegmentation *m_segmentation;
private slots:
	void keyPressEvent(QKeyEvent *keyEvent);
	void closeEvent();

};


//class CWorker: private QThread {
//public:
//	CWorker(){};
//private:
//	void run(){};
//};

}



