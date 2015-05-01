#pragma once
#include <opencv2/opencv.hpp>
#include "WindowManager.h"
#include "Types.h"
#include "PoseRecognition.h"

namespace iez {

class HandTracker;
class PoseRecognition;

class HandTracker {

public:
	explicit HandTracker(bool debug = false);
	~HandTracker();

	void process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId);

	class Data;
	Data data() const;

	class TemporaryResult;
	TemporaryResult temporaryResult() const;

	inline bool isDebug() const { return m_bDebug; }

private:


	void distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT) const;
	void findHandCenter(const cv::Mat &handDT, cv::Point &maxDTPoint) const;

	float findHandCenterRadius(const cv::Point &maxDTPoint,
									  const std::vector<cv::Point> contour) const;


	void findPalm(cv::Mat &binaryPalmMask,
						 std::vector<cv::Point> &palmContour,
						 const cv::Mat &binaryHand,
						 const std::vector<cv::Point> &contour,
						 const cv::Point &palmCenter,
						 const float palmRadius) const;

	bool findWrist(const std::vector<cv::Point> &palmContour,
						  const cv::Point &palmCenter,
						  const float palmRadius,
						  wristpair_t& outputWrist) const;

	void findFingers(cv::Mat &binaryFingersMask,
							std::vector<std::vector<cv::Point> > &fingersContours,
							const cv::Mat &binaryHand,
							const cv::Mat &palmMask) const;

	QList<cv::Point> findFingertip(const cv::RotatedRect &rotRect,
										  const float palmRadius,
										  const cv::Point &palmCenter) const;

	wristpair_t wristPairFix(cv::Point palmCenter, float palmRadius, cv::Point wristMiddle) const;

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
	public:
		cv::Mat originalColor;
		cv::Mat originalDepth;

		cv::Mat depthMaskedImage;

		QList<cv::Mat> medianList;

		cv::Mat distanceTransform;

		cv::Mat handMask;
		std::vector<cv::Point> handContour;

		float palmRadius;
		cv::Point palmCenter;
		cv::Mat palmMask;
		std::vector<cv::Point> palmContour;


		QList<std::vector<cv::Point> > fingerContoursIgnoredList;
		cv::Mat fingersMask;
		std::vector<std::vector<cv::Point> > fingersContours;

		QList<cv::Point> fingertipsNonSorted;

		cv::Mat result;

	};
private:
	Data m_data;
	const bool m_bDebug;
	mutable TemporaryResult m_temp;
	long m_lastImageId;
};

}
