#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <QList>
#include <QLinkedList>
#include <QVector>
namespace iez {


class GeneralizedHough
{
public:
	GeneralizedHough();
	~GeneralizedHough();
	void process();
	void buildRTableCircle(const float radius, const cv::Point centerDisplacement);
	cv::Point detectRTableCircle();
private:
	class HoughItem {
	public:
		cv::Point point;
	};

	cv::Mat m_img;
	QVector<QLinkedList<HoughItem> > m_rTable;
	float m_rTableStep;
	std::vector<std::vector<cv::Point> > m_contours;
};

}
