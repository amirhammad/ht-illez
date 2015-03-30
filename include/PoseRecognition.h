#pragma once
#include "Types.h"
#include <opencv2/opencv.hpp>
#include <QSettings>

namespace iez {
class PoseRecognition {
public:
	enum POSE {
		POSE_0,
		POSE_1,
		POSE_2,
		POSE_3,
		POSE_4,
		POSE_5,
	};

	PoseRecognition();

	void learn(const POSE pose,
			   const wristpair_t &wrist,
			   const QList<cv::Point> &fingertips);

	POSE categorize(const wristpair_t &wrist,
					const QList<cv::Point> &fingertips);
private:
	QSettings m_settings;

};
}

