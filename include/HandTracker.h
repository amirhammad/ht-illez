#pragma once
#include <opencv2/opencv.hpp>
#include "WindowManager.h"
#include "ColorSegmentation.h"
#include "Types.h"
#include "PoseRecognition.h"

namespace iez {

class HandTracker;
class PoseRecognition;

class HandTracker {

public:
	HandTracker();
	void invokeProcess(const cv::Mat &bgr, const cv::Mat &depth, const int imageId = 0);

private:
	class Data;

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
							std::vector<std::vector<cv::Point> > &fingersContours,
							const cv::Mat &binaryHand,
							const cv::Mat &palmMask,
							Data &data);
	static QList<cv::Point> findFingertip(const cv::RotatedRect &rotRect,
										  const float palmRadius,
										  const cv::Point &palmCenter);

private:

	// Comparator for findWrist
	struct C1 {
	private:
		static QVector<float> *m_vector;
	public:
		/// less
		bool operator()(int a, int b) const { return (*m_vector)[a] < (*m_vector)[b]; }
		static void setVector(QVector<float> *vector);
	};

	class Data {
	public:
		void setWrist(wristpair_t wrist);
		wristpair_t wrist() const;
		void setFingertips(QList<cv::Point> fingertips);
		QList<cv::Point> fingertips() const;

		PoseRecognition *pose();
	private:
		wristpair_t m_wrist;
		QList<cv::Point> m_fingertips;

		PoseRecognition m_pose;
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
