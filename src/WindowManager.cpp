#include "WindowManager.h"
#include "qcustomplot.h"
namespace iez {
void CWindowManager::imShow(const char *name, const cv::Mat& image)
{
	m_mutex.lock();
	m_imShowMap[name].second = Mat2QImage(image);
	m_mutex.unlock();
	QMetaObject::invokeMethod(this, "on_imShow", Qt::QueuedConnection, QGenericArgument("const char*", &name));

}

void CWindowManager::plot(const char *name,
		const QVector<double> &x,
		const QVector<double> &y)
{
//	QMainWindow *w = new QMainWindow();
//	w->setFixedSize(QSize(640, 480));

	m_plotMap[name].x = x;
	m_plotMap[name].y = y;

	QMetaObject::invokeMethod(this, "on_plot", Qt::QueuedConnection, QGenericArgument("const char*", &name));
}

void CWindowManager::on_plot(const char *name)
{
	struct plotMapData *data= &m_plotMap[name];

	if (!data->widget) {
		CWindow *widget = new CWindow();
		widget->setWindowTitle(name);
		widget->setFixedSize(QSize(640, 480));
		connect(widget, SIGNAL(keyPressed(QKeyEvent *)), this, SLOT(keyPressEvent(QKeyEvent *)));
		connect(widget, SIGNAL(closed()), this, SLOT(closeEvent()));
		m_plotMap[name].widget = widget;
	}

	QCustomPlot *customPlot = new QCustomPlot(data->widget);

//	customPlot->setViewport(QRect(QPoint(0,0), QSize(640,480)));
//	customPlot->setBaseSize(QSize(640,480);
	customPlot->setFixedSize(640, 480);
	// generate some data:

	// create graph and assign data to it:
	customPlot->addGraph();
	customPlot->graph(0)->setData(data->x, data->y);
	// give the axes some labels:
	customPlot->xAxis->setLabel("x");
	customPlot->yAxis->setLabel("y");
	// set axes ranges, so we see all data:
	customPlot->xAxis->setRange(-1, 1);
	customPlot->yAxis->setRange(0, 1);
	customPlot->replot();
	data->widget->show();
}
QImage CWindowManager::Mat2QImage(cv::Mat const& src)
{
	using namespace cv;
     cv::Mat temp; // make the same cv::Mat
     if (src.type() == CV_8UC1) {
    	cvtColor(src, temp,COLOR_GRAY2RGB); // cvtColor Makes a copt, that what i need
     } else {
    	cvtColor(src, temp, COLOR_BGR2RGB); // cvtColor Makes a copt, that what i need
     }
     QImage dest((const uchar *) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
     dest.bits(); // enforce deep copy, see documentation
     // of QImage::QImage ( const uchar * data, int width, int height, Format format )
     return dest;
}

cv::Mat CWindowManager::QImage2Mat(QImage const& src)
{
	using namespace cv;
	cv::Mat tmp(src.height(),src.width(),CV_8UC3,(uchar*)src.bits(),src.bytesPerLine());
	cv::Mat result; // deep copy just in case (my lack of knowledge with open cv)
	cvtColor(tmp, result,COLOR_BGR2RGB);
	return result;
}

CWindowManager::CWindowManager()
{
	qRegisterMetaType<const char*>("const char*");
}

void CWindowManager::on_imShow(const char * str)
{
	m_mutex.lock();
	if (!m_imShowMap[str].first) {
		CWindow *mapLabel = new CWindow();

		mapLabel->setFixedSize(640,480);
		mapLabel->setWindowTitle(str);
		mapLabel->show();
		connect(mapLabel, SIGNAL(keyPressed(QKeyEvent *)), this, SLOT(keyPressEvent(QKeyEvent *)));
		connect(mapLabel, SIGNAL(closed()), this, SLOT(closeEvent()));
		m_imShowMap[str].first = mapLabel;
	}

	m_imShowMap[str].first->setPixmap(QPixmap::fromImage(m_imShowMap[str].second));
	m_mutex.unlock();
}

}
