#pragma once

#include <opencv2/opencv.hpp>
#include <map>
#include <QtGui>
#include <string>
#include "HandTracker.h"
#include "ImageSource.h"
#include "WindowManager.h"

namespace iez {


class Processing: private QThread
{
	Q_OBJECT
public:
	explicit Processing(ImageSource &imgsrc);
	~Processing(void);

	int init();
	void process(const cv::Mat &bgr, const cv::Mat &depth);
private:
	void filterDepth(cv::Mat &dst, const cv::Mat &src, int near, int far);

	void processHSVFilter(const cv::Mat &orig);
	void run();


	WindowManager m_window;

	CHandTracker m_handTracker;
	ImageSource &m_imageSource;

	bool m_calculateHandTracker;
private slots:
	void keyPressEvent(QKeyEvent *keyEvent);
	void closeEvent();
};

class CWorker: private QThread {
public:
	CWorker(){};
private:
	void run(){};
};

}



