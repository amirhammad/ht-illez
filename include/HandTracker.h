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
	~HandTracker();

	void process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId);

	class Data;
	Data data() const;

	class TemporaryResult;
	TemporaryResult temporaryResult() const;


private:


	static void distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT);
	static void findHandCenter(const cv::Mat &handDT, cv::Point &maxDTPoint);

	static float findHandCenterRadius(const cv::Point &maxDTPoint,
									  const std::vector<cv::Point> contour);


	static void findPalm(cv::Mat &binaryPalmMask,
						 std::vector<cv::Point> &palmContour,
						 const cv::Mat &binaryHand,
						 const std::vector<cv::Point> &contour,
						 const cv::Point &palmCenter,
						 const float palmRadius);

	static bool findWrist(const std::vector<cv::Point> &palmContour,
						  const cv::Point &palmCenter,
						  const float palmRadius,
						  const Data &data,
						  wristpair_t& outputWrist);

	static void findFingers(cv::Mat &binaryFingersMask,
							std::vector<std::vector<cv::Point> > &fingersContours,
							const cv::Mat &binaryHand,
							const cv::Mat &palmMask,
							const Data &data);

	static QList<cv::Point> findFingertip(const cv::RotatedRect &rotRect,
										  const float palmRadius,
										  const cv::Point &palmCenter);

	static wristpair_t wristPairFix(cv::Point palmCenter, float palmRadius, cv::Point wristMiddle);

private:
	static void orderFingertipsByAngle(wristpair_t wrist, QList<cv::Point> &fingertips);
public:
	class Data {
	public:
		void setWrist(wristpair_t wrist);
		wristpair_t wrist() const;

		void setFingertips(QList<cv::Point> fingertips);
		QList<cv::Point> fingertips() const;

		cv::Point palmCenter() const;
		void setPalmCenter(const cv::Point &palmCenter);

		float palmRadius() const;
		void setPalmRadius(float palmRadius);

	private:
		cv::Point m_palmCenter;
		float m_palmRadius;
		wristpair_t m_wrist;
		QList<cv::Point> m_fingertips;
	};

	class TemporaryResult {

	};
private:
	Data m_data;
	TemporaryResult m_temporaryResult;
};

}
