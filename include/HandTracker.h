/*
 * This file is part of the project HandTrackerApp - ht-illez
 *
 * Copyright (C) 2015 Amir Hammad <amir.hammad@hotmail.com>
 *
 *
 * HandTrackerApp - ht-illez is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#pragma once
#include <opencv2/opencv.hpp>
#include "Types.h"
#include <QList>
#include <QVector>

namespace iez {

class HandTracker;
class PoseRecognition;

class HandTracker {

public:
	explicit HandTracker();
	~HandTracker();

	/**
	 * @brief process
	 * @param bgr BGR formatted image
	 * @param depth	Depth map of the same size as bgr
	 * @param imageId monotonically rising ID of current image
	 */
	void process(const cv::Mat &bgr, const cv::Mat &depth, const int imageId);

	/**
	 * @brief data
	 * @return returns result of process
	 */
	class Data;
	Data data() const;

	class TemporaryResult;
	TemporaryResult temporaryResult() const;

	inline bool isDebug() const { return m_bDebug; }

private:
	static inline void imshow(QString name, cv::Mat mat);

	void distanceTransform(const cv::Mat &binaryHandFiltered, cv::Mat &handDT) const;
	void findHandCenter(const cv::Mat &handDT, cv::Point &maxDTPoint) const;

	float findHandCenterRadius(const cv::Point &maxDTPoint,
									  const std::vector<cv::Point> &contour) const;


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
		Data(){}
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
		QVector<double> fingertipsNormalized;

	};
private:
	Data m_data;
	bool m_bDebug;
	mutable TemporaryResult m_temp;
	long m_lastImageId;
};

}
