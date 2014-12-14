#pragma once
#include <opencv2/opencv.hpp>
#include "HandTracker.h"
namespace iez {
class CProcessing
{
public:
	CProcessing(void);
	~CProcessing(void);

	void process(const cv::Mat &bgr, const cv::Mat &depth);
	void findHandFromCenter(const cv::Mat &bgr, const cv::Mat &depth);
private:
	void filterDepth(cv::Mat &dst, const cv::Mat &src, int near, int far);

	CHandTracker handTracker;
};

}
