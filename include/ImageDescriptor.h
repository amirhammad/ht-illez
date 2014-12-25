#pragma once
#include <QtGui>
#include <iostream>
#include <list>
#include "WindowManager.h"
#include <opencv2/opencv.hpp>

namespace iez {
class ImageDescriptorImage : public QLabel {
	Q_OBJECT
public:
	ImageDescriptorImage();

private:
	cv::VideoCapture m_cap;
	QImage image;
	std::list<QPoint> m_pointList;
	int m_pointListCurrent;
	bool pressed;

	QImage m_image;
public slots:
	void refresh();
	void refresh(const QImage &image);
private slots:
	void on_fileSelected(const QString &file);
	void mousePressEvent(QMouseEvent *ev);
	void mouseReleaseEvent(QMouseEvent *ev);
	void mouseMoveEvent(QMouseEvent *ev);
	void paintEvent(QPaintEvent *ev);
	void keyPressEvent(QKeyEvent *ev);
signals:
	void polygonSelected(QPolygon);


};
class ImageDescriptor : private QObject {
	Q_OBJECT
public:
	ImageDescriptor();
	~ImageDescriptor();

public slots:
	void refresh() {
		m_backgroundImage->refresh();
	}

	void on_polygonSelected(QPolygon polygon);

private:
	void addPolygon(QPolygon);
	std::list<QPolygon> m_polygonsList;
	ImageDescriptorImage *m_backgroundImage;
	QWidget *master;
	QWidget *m_polygonsWidget;
//private slots:

};


}
