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


#include "ImageSourceOpenNI.h"
#include <QApplication>
#include <QDebug>

using namespace iez;
using namespace cv;

#define FILE_PLAY_SPEED (30)
#define MAX_FAIL_COUNT (20)
ImageSourceOpenNI::ImageSourceOpenNI(int fps)
:	m_width(640)
,	m_height(480)
,	m_fps(fps)
,	m_initialized(false)
,	m_failCount(0)
{
	openni::OpenNI::initialize();
	m_thread = new QThread(QApplication::instance()->thread());
	moveToThread(m_thread);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(update()));
	m_thread->start();
}


ImageSourceOpenNI::~ImageSourceOpenNI(void)
{
	qDebug("X2");
//	moveToThread(QApplication::instance()->thread());

	m_thread->exit();
	// If thread does not terminate within 10s, terminate it
	if (!m_thread->wait(10000)) {
		m_thread->terminate();
		m_thread->wait();
	}
	m_thread->deleteLater();

	if (m_initialized) {
		m_depthStream.stop();
		m_colorStream.stop();
		m_colorFrame.release();
		m_depthFrame.release();
		m_colorStream.destroy();
		m_depthStream.destroy();
		device.close();
	}

	openni::OpenNI::shutdown();
	qDebug("X2 end");
}

int ImageSourceOpenNI::deviceInit(QString deviceURI)
{
	openni::Status rc;

	rc = openni::OpenNI::initialize();

	if (deviceURI.isEmpty()) {
		rc = device.open(openni::ANY_DEVICE);
	} else {
		rc = device.open(deviceURI.toStdString().c_str());
	}

	if (rc != openni::STATUS_OK)
	{
		printf("Device open failed:\n%s\n", openni::OpenNI::getExtendedError());
		openni::OpenNI::shutdown();
		return openni::STATUS_ERROR;
	}

	if (device.isFile()) {
		m_fps = FILE_PLAY_SPEED;
		device.getPlaybackControl()->setSpeed(m_fps);
		device.getPlaybackControl()->setRepeatEnabled(true);
	}


	rc = m_depthStream.create(device, openni::SENSOR_DEPTH);
	if (rc == openni::STATUS_OK)
	{
		rc = m_depthStream.start();
		if (rc != openni::STATUS_OK)
		{
			printf("Couldn't start depth stream:\n%s\n", openni::OpenNI::getExtendedError());
			m_depthStream.destroy();
		}
	}
	else
	{
		printf("Couldn't find depth stream:\n%s\n", openni::OpenNI::getExtendedError());
	}

	rc = m_colorStream.create(device, openni::SENSOR_COLOR);
	if (rc == openni::STATUS_OK)
	{
		rc = m_colorStream.start();
		if (rc != openni::STATUS_OK)
		{
			printf("Couldn't start color stream:\n%s\n", openni::OpenNI::getExtendedError());
			m_colorStream.destroy();
		}
	}
	else
	{
		printf("Couldn't find color stream:\n%s\n", openni::OpenNI::getExtendedError());
	}

	if (!m_depthStream.isValid() || !m_colorStream.isValid())
	{
		printf("No valid streams. Exiting\n");
		openni::OpenNI::shutdown();
		return openni::STATUS_ERROR;
	}

	device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
	device.setDepthColorSyncEnabled(true);

	return 0;
}

int ImageSourceOpenNI::streamInit()
{
	openni::VideoMode depthVideoMode;
	openni::VideoMode colorVideoMode;

	if (m_depthStream.isValid() && m_colorStream.isValid())
	{
		depthVideoMode = m_depthStream.getVideoMode();
		colorVideoMode = m_colorStream.getVideoMode();

		int depthWidth = depthVideoMode.getResolutionX();
		int depthHeight = depthVideoMode.getResolutionY();
		int colorWidth = colorVideoMode.getResolutionX();
		int colorHeight = colorVideoMode.getResolutionY();

		if (depthWidth == colorWidth && depthHeight == colorHeight) {
			m_width = depthWidth;
			m_height = depthHeight;
		} else {
			printf("Error - expect color and depth to be in same resolution: D: %dx%d, C: %dx%d\n",
				depthWidth, depthHeight,
				colorWidth, colorHeight);
			return openni::STATUS_ERROR;
		}
	} else if (m_depthStream.isValid()) {

		depthVideoMode = m_depthStream.getVideoMode();

		m_width = depthVideoMode.getResolutionX();
		m_height = depthVideoMode.getResolutionY();
	} else if (m_colorStream.isValid()) {
		colorVideoMode = m_colorStream.getVideoMode();

		m_width = colorVideoMode.getResolutionX();
		m_height = colorVideoMode.getResolutionY();
	} else {
		printf("Error - expects at least one of the streams to be valid...\n");
		return openni::STATUS_ERROR;
	}

	m_streams[0] = &m_depthStream;
	m_streams[1] = &m_colorStream;

	return 0;
}

void ImageSourceOpenNI::readDepth()
{
	QWriteLocker l(&depth_rwlock);
	m_depthStream.readFrame(&m_depthFrame);
	m_sequence = max<int>(m_sequence, m_depthFrame.getFrameIndex());//bug(overflow in ~700 days)
	if (m_depthFrame.isValid()) {
		memcpy(m_depthMat.data, m_depthFrame.getData(), m_depthFrame.getDataSize());
	}
}

void ImageSourceOpenNI::readColor()
{
	QWriteLocker l(&color_rwlock);
	m_colorStream.readFrame(&m_colorFrame);
	m_sequence = max<int>(m_sequence, m_colorFrame.getFrameIndex());//bug(overflow in ~700 days)
	if (m_colorFrame.isValid()) {
		memcpy(m_colorMat.data, m_colorFrame.getData(), m_colorFrame.getDataSize());
	}
}

void ImageSourceOpenNI::pause(bool p)
{
	qDebug("pause received");
	if (p) {
		if (m_timer.isActive()) {
			m_timer.stop();
		}
	} else {
		m_timer.start();
	}
}

void ImageSourceOpenNI::update() 
{
	int changedIndex;

	for (int i = 0; i < 2; i++) {
		openni::Status rc = openni::OpenNI::waitForAnyStream(m_streams, 2, &changedIndex, 0);
		if (rc != openni::STATUS_OK) {
			m_failCount++;
			if (m_failCount > MAX_FAIL_COUNT) {
//				QApplication::exit();
			}
//			printf("Wait failed\n");
			return;
		} else {
			m_failCount = 0;
		}

		switch (changedIndex) {
		case 0:
		{
			readDepth();
		}
			break;
		case 1:
		{
			readColor();
		}
			break;
		default:
			printf("Error in wait\n");
		}
	}
	emit frameReceived();
}


bool ImageSourceOpenNI::init(QVariant args)
{
	QString deviceURI;
	if (args.toString().isEmpty()) {
		deviceURI = QString();
	} else {
		deviceURI = args.toString();
		qDebug() << "Opening file " << deviceURI;
	}

	if (m_initialized) {
		return true;
	}

	try {
		if (openni::STATUS_ERROR == deviceInit(deviceURI)) {
			m_initialized = false;
			return false;
		}
		if (openni::STATUS_ERROR == streamInit()) {
			m_initialized = false;
			return false;
		}
	} catch (...) {
		return false;
	}

	m_depthMat.create(m_height, m_width, CV_16UC1);
	m_colorMat.create(m_height, m_width, CV_8UC3);
	m_initialized = true;

	m_timer.setSingleShot(false);
	m_timer.start(1000/m_fps);

	return true;
}

bool ImageSourceOpenNI::isInitialized()
{
	return m_initialized;
}

cv::Mat ImageSourceOpenNI::getDepthMat() const
{
	QReadLocker l(&depth_rwlock);
	return m_depthMat;
}

cv::Mat ImageSourceOpenNI::getColorMat() const
{
	QReadLocker l(&color_rwlock);
	return m_colorMat;
}

openni::VideoStream& iez::ImageSourceOpenNI::getColorStream()
{
	return m_colorStream;
}

openni::VideoStream& iez::ImageSourceOpenNI::getDepthStream()
{
	return m_depthStream;
}
