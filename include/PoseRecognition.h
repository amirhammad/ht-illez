#pragma once
#include "Types.h"
#include <opencv2/opencv.hpp>
#include <QSettings>
#include <QObject>
#include <opennn.h>

namespace iez {

class PoseRecognition : QObject{
Q_OBJECT

public:
	enum POSE {
		POSE_0 = 0,
		POSE_1 = 1,
		POSE_2 = 2,
		POSE_3 = 3,
		POSE_4 = 4,
		POSE_5 = 5,

		POSE_END = 6
	};

	PoseRecognition();

	void learnNew(const POSE pose,
			   const cv::Point palmCenter,
			   const float palmRadius,
			   const wristpair_t &wrist,
			   const QList<cv::Point> &fingertips);
	void learn();
	POSE categorize(const wristpair_t &wrist,
					const QList<cv::Point> &fingertips);
private:
	void teachNN();
	static OpenNN::Vector<double> constructFeatureVector(	const cv::Point palmCenter,
															const float palmRadius,
															const wristpair_t &wrist,
															const QList<cv::Point> &fingertips,
															const PoseRecognition::POSE pose);
	void appendToMatrix(OpenNN::Vector<double> vec);
	OpenNN::Matrix<double> m_matrix;
	QSettings *m_settings;


};
}

