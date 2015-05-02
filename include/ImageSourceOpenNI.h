/*
 * This file is part of the project HandTrackerApp - ht-illez
 * hosted at http://github.com/amirhammad/ht-illez
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
#include "ImageSource.h"
#include <opencv2/opencv.hpp>

#include <OpenNI.h>

#include <QtCore/qthread.h>
#include <QtCore/qtimer.h>
#include <qreadwritelock.h>
#include <QKeyEvent>

namespace iez {
class ImageSourceOpenNI : public ImageSource
{
	Q_OBJECT
public:
	ImageSourceOpenNI(int fps = 30);
	virtual ~ImageSourceOpenNI(void);

	bool init(QVariant args = QVariant());
	bool isInitialized();


	cv::Mat getDepthMat() const;
	cv::Mat getColorMat() const;

	openni::VideoStream& getColorStream();
	openni::VideoStream& getDepthStream();
private:
//	void run(); // Overriden QThread run


	int deviceInit(QString deviceURI);
	int streamInit(void);

	void readDepth();
	void readColor();
	int m_width, m_height;

	cv::Mat m_depthMat;
	cv::Mat m_colorMat;
	openni::Device device;
	openni::VideoStream m_depthStream, m_colorStream;
	openni::VideoStream *m_streams[2];
	openni::VideoFrameRef m_depthFrame, m_colorFrame;

	QTimer m_timer;
	mutable QReadWriteLock depth_rwlock;
	mutable QReadWriteLock color_rwlock;

	int m_fps;
	bool m_initialized;
	QThread *m_thread;
	int m_failCount;
public slots:
	void pause(bool p = true);
private slots:
	void update();
};


}
