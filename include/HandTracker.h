#pragma once
#include <opencv2/opencv.hpp>
#include "WindowManager.h"
#include "ColorSegmentation.h"
namespace iez {
class HandTracker;

class HandTracker {

public:
	HandTracker();
	void invokeProcess(const cv::Mat &bgr, const cv::Mat &depth, const int imageId = 0);

private:
	class Data;
	typedef std::pair<cv::Point, cv::Point> wristpair;

	static void process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId, Data &data);

	static void distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT);
	static void findHandCenter(const cv::Mat &handDT, cv::Point &maxDTPoint);

	static float findHandCenterRadius(const cv::Mat &binaryHandFiltered,
									  const cv::Point &maxDTPoint,
									  const std::vector<cv::Point> contour);


	static void findPalm(cv::Mat &binaryPalmMask,
						 std::vector<cv::Point> &palmContour,
						 const cv::Mat &binaryHand,
						 const std::vector<cv::Point> &contour,
						 const cv::Point &palmCenter,
						 const float palmRadius);

	static void findWrist(const std::vector<cv::Point> &palmContour,
						  const cv::Point &handCenter,
						  Data &data);

	static void findFingers(cv::Mat &binaryFingersMask,
							const cv::Mat &binaryHand,
							const cv::Mat &palmMask,
							Data &data);

private:

	class Data {
	public:
		void setWrist(wristpair wrist);
		wristpair wrist() const;

	private:
		wristpair m_wrist;

		mutable QMutex m_mutex;
	};

	class Worker : public QRunnable {
	public:
		Worker(const cv::Mat bgr,
			   const cv::Mat depth,
			   const int imageId,
			   Data &data);
		void run();
		static int runningThreads();
	private:
		const cv::Mat m_bgr;
		const cv::Mat m_depth;
		const int m_imageId;
		Data &m_data;

		static int m_runningThreads;
	};

private:
	Data m_data;
	int m_maxThreads;
};

}
