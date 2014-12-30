#pragma once

#include <opencv2/opencv.hpp>
#include <map>
#include <QtGui>
#include <string>
#include "HandTracker.h"
#include "ImageSourceFreenect.h"
#include "WindowManager.h"

namespace iez {


class Processing: private QThread
{
	Q_OBJECT
public:
	explicit Processing(ImageSourceFreenect &imgsrc);
	~Processing(void);

private:
	void processDepthFiltering(const cv::Mat &bgr, const cv::Mat &depth);
	void filterDepth(cv::Mat &dst, const cv::Mat &src, int near, int far);


	void processHSVFilter(const cv::Mat &orig);
	void run();
	void process(const cv::Mat &bgr, const cv::Mat &depth);


//	CHandTracker m_handTracker;
	ImageSourceFreenect &m_imageSource;

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



