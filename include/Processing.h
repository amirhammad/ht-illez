#pragma once
#include <opencv2/opencv.hpp>
namespace iez {
class CProcessing
{
public:
	CProcessing(void);
	~CProcessing(void);

	void process(const cv::Mat &rgb, const cv::Mat &depth);
private:
	void filterDepth(cv::Mat &dst, const cv::Mat &src, int near, int far);
};

}
