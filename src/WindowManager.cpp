#include "WindowManager.h"

#include <QCoreApplication>

namespace iez {
void WindowManager::imShow(const QString name, const cv::Mat& image)
{
	imShow(name, Mat2QImage(image));
}

void WindowManager::imShow(const QString name, const QImage& image)
{
	QMutexLocker l(&m_mutex);
	struct imShowMapData * data = &m_imShowMap[name];//.image = image;
	data->image = image;
	QMetaObject::invokeMethod(this, "on_imShow", Qt::QueuedConnection, QGenericArgument("const QString", &name));
}

QImage WindowManager::Mat2QImage(const cv::Mat& src)
{
	using namespace cv;
	 cv::Mat temp; // make the same cv::Mat
	 if (src.type() == CV_8UC1) {
	cvtColor(src, temp, COLOR_GRAY2RGB); // cvtColor Makes a copy, that what i need
	 } else if (src.type() == CV_8UC3) {
	cvtColor(src, temp, COLOR_BGR2RGB); // cvtColor Makes a copy, that what i need
	 }

	 QImage dest((const uchar *) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
	 dest.bits(); // enforce deep copy, see documentation
	 // of QImage::QImage ( const uchar * data, int width, int height, Format format )
	 return dest;
}

cv::Mat WindowManager::QImage2Mat(QImage const& src)
{
	using namespace cv;
	cv::Mat tmp(src.height(),src.width(),CV_8UC3,(uchar*)src.bits(),src.bytesPerLine());
	cv::Mat result; // deep copy just in case (my lack of knowledge with open cv)
	cvtColor(tmp, result,COLOR_BGR2RGB);
	return result;
}

WindowManager::WindowManager()
{
	moveToThread(QCoreApplication::instance()->thread());
}

WindowManager::~WindowManager()
{
	qDebug("WM desctruction");
	foreach (const QString k, m_imShowMap.keys()) {
		m_imShowMap.value(k).widget->deleteLater();
	}
}

void WindowManager::on_imShow(const QString str)
{
	QMutexLocker l(&m_mutex);
	if (!m_imShowMap[str].widget) {
		Window *mapLabel = new Window();
		connect(mapLabel, SIGNAL(keyPressed(QKeyEvent *)), this, SIGNAL(keyPressed(QKeyEvent *)));
		m_imShowMap[str].widget = mapLabel;
	}

	Window *mapLabel = m_imShowMap[str].widget;
	mapLabel->setFixedSize(m_imShowMap[str].image.width(),m_imShowMap[str].image.height());
	mapLabel->setWindowTitle(str);
	mapLabel->show();


	mapLabel->setPixmap(QPixmap::fromImage(m_imShowMap[str].image));
}

}
