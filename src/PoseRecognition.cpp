#include "PoseRecognition.h"
namespace iez {

PoseRecognition::PoseRecognition()
	:	m_settings("pose_recognition.ini", QSettings::IniFormat){
}

void PoseRecognition::learn(const PoseRecognition::POSE pose, const wristpair_t &wrist, const QList<cv::Point> &fingertips)
{

}

PoseRecognition::POSE PoseRecognition::categorize(const wristpair_t &wrist, const QList<cv::Point> &fingertips)
{
	return POSE_0;
}

}



