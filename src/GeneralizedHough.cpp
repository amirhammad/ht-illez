#include "GeneralizedHough.h"
#include "WindowManager.h"
#include "Processing.h"
#include <QDebug>
namespace iez {
GeneralizedHough::GeneralizedHough()
{

}

GeneralizedHough::~GeneralizedHough()
{

}

void GeneralizedHough::process()
{
	buildRTableCircle(200, cv::Point(50, 5));
	detectRTableCircle();
	cv::waitKey(-1);
}



void GeneralizedHough::buildRTableCircle(const float radius, const cv::Point centerDisplacement)
{
	cv::Mat img = cv::Mat::zeros(radius*3, radius*3, CV_8UC1);
	const cv::Point center = cv::Point(radius*1.5, radius*1.5);
	const cv::Point ref = center + centerDisplacement;
	cv::ellipse(img, center, cv::Size(radius, radius), 0, 0, 360, cv::Scalar(255), 1);
	cv::ellipse(img, ref, cv::Size(1, 1), 0, 0, 360, cv::Scalar(255), 1);

	m_img = img;
	cv::Mat tmp;
	img.copyTo(tmp);
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(tmp, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	m_contours = contours;
	cv::imshow("d", img);


	const int rTableSize = 200;
	QVector<QLinkedList<HoughItem> > rTable(rTableSize);
	const float step = 2*M_PI/rTableSize;
	m_rTableStep = step;
	for (int i = 1; i < contours[0].size() - 1; i++) {
		float phi;
		if (contours[0][i + 1].x > contours[0][i - 1].x) {
			phi = atan2((contours[0][i + 1].y - contours[0][i - 1].y), (contours[0][i + 1].x - contours[0][i - 1].x));
		} else {
			phi = atan2((contours[0][i - 1].y - contours[0][i + 1].y), (contours[0][i - 1].x - contours[0][i + 1].x));
		}
//		if (phi < 0) {
			phi += M_PI;
//		}

//		float alpha;
//		if (ref.x > contours[0][i].x) {
//			alpha = atan2(ref.y - contours[0][i].y, ref.x - contours[0][i].x);
//		} else {
//			alpha = atan2(contours[0][i].y - ref.y, contours[0][i].x - ref.x);
//		}
//		if (alpha < 0) {
//			alpha += M_PI;
//		}
		HoughItem item;
		item.point = ref - contours[0][i];

//		if (item.point.x < 0) {
//			qDebug() << "[]"<< QString::number(ref.x) << "  " << QString::number(ref.y);
//			qDebug() << QString::number(contours[0][i].x) << "  " << QString::number(contours[0][i].y);
////			qt_assert("x failed", __FILE__, __LINE__);
//		}
//		if (item.point.y < 0) {
//			qDebug() << "[]"<< QString::number(ref.x) << "  " << QString::number(ref.y);
//			qDebug() << QString::number(contours[0][i].x) << "  " << QString::number(contours[0][i].y);
////			qt_assert("y failed", __FILE__, __LINE__);
//		}

		rTable[phi/step].append(item);
	}

	QStringList out;
	for (int i = 0; i < rTable.size(); i++) {
		QStringList line;
		QLinkedListIterator<HoughItem> it(rTable[i]);
		while (it.hasNext()) {
			const HoughItem &item = it.next();
			line << QString::number(item.point.x).append(QString(",").append(QString::number(item.point.y)));
		}
		out << line.join("|");
	}
//	qDebug() << out;
	m_rTable = rTable;
//	WindowManager::getInstance().imShow("rTableCircle", img);
}

cv::Point GeneralizedHough::detectRTableCircle()
{
	cv::Mat acc = cv::Mat::zeros(m_img.rows, m_img.cols, CV_16UC1);
	for (int i = 1; i < m_contours[0].size() - 1; i++) {
		const cv::Point &currPoint = m_contours[0][i];
		float phi;
		if (m_contours[0][i + 1].x > m_contours[0][i - 1].x) {
			phi = atan2((m_contours[0][i + 1].y - m_contours[0][i - 1].y), (m_contours[0][i + 1].x - m_contours[0][i - 1].x));
		} else {
			phi = atan2((m_contours[0][i - 1].y - m_contours[0][i + 1].y), (m_contours[0][i - 1].x - m_contours[0][i + 1].x));
		}
//		if (phi < 0) {
			phi += M_PI;
//		}
		QLinkedListIterator<HoughItem> it(m_rTable[phi/m_rTableStep]);
		while (it.hasNext()) {
			const HoughItem &item = it.next();
			cv::Point ref = currPoint + item.point;
			if (ref.y >= 0 && ref.x >= 0 && ref.y <= m_img.rows && ref.x <= m_img.cols) {
				qDebug() << "x";
				acc.at<uint16_t>(ref.y, ref.x)++;
			}
		}
	}
	cv::Mat sh;
	acc.convertTo(sh, CV_8UC1, 1.f);
	imshow("x", sh);
}


}
