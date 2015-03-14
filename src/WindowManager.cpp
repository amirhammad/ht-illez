#include "WindowManager.h"
#include "qcustomplot.h"
#include <QCoreApplication>

namespace iez {
void WindowManager::imShow(const QString name, const cv::Mat& image)
{
	imShow(name, Mat2QImage(image));
}

void WindowManager::imShow(const QString name, const QImage& image)
{
	m_mutex.lock();
	struct imShowMapData * data = &m_imShowMap[name];//.image = image;
	data->image = image;
	m_mutex.unlock();
	QMetaObject::invokeMethod(this, "on_imShow", Qt::AutoConnection, QGenericArgument("const QString", &name));
}

void WindowManager::plot(const QString name,
		const QVector<double> &x,
		const QVector<double> &y)
{
//	QMainWindow *w = new QMainWindow();
//	w->setFixedSize(QSize(640, 480));
	m_mutex.lock();
	m_plotMap[name].x = QVector<double>(x);
	m_plotMap[name].y = QVector<double>(y);
	m_mutex.unlock();
	QMetaObject::invokeMethod(this, "on_plot", Qt::AutoConnection, QGenericArgument("const QString", &name));
}


void WindowManager::plot(const QString name,
		const std::vector<double> &x,
		const std::vector<double> &y)
{
	plot(name, QVector<double>::fromStdVector(x), QVector<double>::fromStdVector(y));
}

void WindowManager::on_plot(const QString name)
{
	m_mutex.lock();
	struct plotMapData *data= &m_plotMap[name];

	if (!data->widget) {
		Window *widget = new Window();
		widget->setWindowTitle(name);
		widget->setFixedSize(QSize(900, 450));
//		connect(widget, SIGNAL(keyPressed(QKeyEvent *)), this, SLOT(keyPressEvent(QKeyEvent *)));
//		connect(widget, SIGNAL(closed()), this, SLOT(closeEvent()));
		data->widget = widget;

		m_plotMap[name].customPlot = new QCustomPlot(data->widget);

	}
	QCustomPlot *customPlot = m_plotMap[name].customPlot;
	customPlot->clearGraphs();
//	customPlot->setViewport(QRect(QPoint(0,0), QSize(640,480)));
//	customPlot->setBaseSize(QSize(640,480);
	customPlot->setFixedSize(800, 400);
	// generate some data:

	// create graph and assign data to it:
	customPlot->addGraph();

	double min[2],max[2];
	min[1]=std::numeric_limits<double>::max();
	max[1]=0;
	foreach(double val, data->y) {
		if (val > max[1] ) max[1] = val;
		if (val < min[1] ) min[1] = val;
	}
	min[0]=std::numeric_limits<double>::max();
	max[0]=0;
	foreach(double val, data->x) {
		if (val > max[0] ) max[0] = val;
		if (val < min[0] ) min[0] = val;
	}


	customPlot->graph(0)->setData(data->x, data->y);
	// give the axes some labels:
	customPlot->xAxis->setLabel("x");
	customPlot->yAxis->setLabel("y");
//	std::cout<<"["<<min[0]<<" "<<min[1]<<"]["<<max[0]<<" "<<max[1]<<"]"<<std::endl;
	// set axes ranges, so we see all data:
	customPlot->xAxis->setRange(0, 1);
	customPlot->yAxis->setRange(-5, 5);
	customPlot->replot();
	data->widget->show();
	m_mutex.unlock();
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

void WindowManager::on_imShow(const QString str)
{
	m_mutex.lock();
	if (!m_imShowMap[str].widget) {
		Window *mapLabel = new Window();
//		mapLabel->moveToThread(QCoreApplication::instance()->thread());
		m_imShowMap[str].widget = mapLabel;
	}

	Window *mapLabel = m_imShowMap[str].widget;
	mapLabel->setFixedSize(m_imShowMap[str].image.width(),m_imShowMap[str].image.height());
	mapLabel->setWindowTitle(str);
	mapLabel->show();

	mapLabel->setPixmap(QPixmap::fromImage(m_imShowMap[str].image));
	m_mutex.unlock();
}

}
