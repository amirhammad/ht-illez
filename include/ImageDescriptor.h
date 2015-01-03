#pragma once
#include <QtGui>
#include <iostream>
#include <list>
#include "WindowManager.h"
#include "ImageSourceFreenect.h"
#include <opencv2/opencv.hpp>

namespace iez {
class ImageDescriptorImage : public QLabel {
	Q_OBJECT
public:
	ImageDescriptorImage(ImageSourceBase *kinect = 0);

private:
	cv::VideoCapture m_cap;
	iez::ImageSourceBase *m_kinect;
	std::list<QPoint> m_pointList;
	int m_pointListCurrent;
	bool pressed;

	QImage m_image;
	std::list<QPolygon> m_polygonList;
	long m_fpsCounter;
	void refresh(const QImage &image);
public slots:
	void refresh();

private slots:
	void mousePressEvent(QMouseEvent *ev);
	void mouseReleaseEvent(QMouseEvent *ev);
	void mouseMoveEvent(QMouseEvent *ev);
	void paintEvent(QPaintEvent *ev);
	void keyPressEvent(QKeyEvent *ev);
signals:
	void polygonSelected(QPolygon);
	void descriptionComplete(const QImage &, const std::list<QPolygon>);


};

class ImageDescriptor : private QObject {
	Q_OBJECT
public:
	ImageDescriptor(ImageSourceBase *kinect = 0);
	~ImageDescriptor();

public slots:
	void refresh() {
		m_backgroundImage->refresh();
	}

private slots:
	void on_descriptionComplete(const QImage& image, const std::list<QPolygon> polygonList);
	void on_descriptionFileSelected(const QString &file);
	void pause();
//	void on_polygonSelected(QPolygon polygon);

private:
	void addPolygon(QPolygon);

	ImageDescriptorImage *m_backgroundImage;
	QWidget *master;
	QWidget *m_polygonsWidget;

	QImage m_image;
	std::list<QPolygon> m_polygonList;

	QTimer m_timer;

//private slots:

};



}
