/*
 * This file is part of the project HandTrackerApp - ht-illez
 *
 * Copyright (C) 2015 Amir Hammad <amir.hammad@hotmail.com>
 *
 *
 * HandTrackerApp - ht-illez is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


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
	ImageDescriptorImage(ImageSource *kinect = 0);

private:
	cv::VideoCapture m_cap;
	iez::ImageSource *m_kinect;
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
	ImageDescriptor(ImageSource *kinect = 0);
	~ImageDescriptor();

public slots:
//	void refresh() {
//		m_backgroundImage->refresh();
//	}

private slots:
	void on_descriptionComplete(const QImage& image, const std::list<QPolygon> &polygonList);
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
