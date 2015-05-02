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


#include "Processing.h"
#include "ImageSource.h"
#include <stdint.h>
#include <string>
#include <math.h>
//#include <QtGui>
//#include <QtCore>
//#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include "ImageSourceArtificial.h"
#include <QApplication>

//using namespace cv;
namespace iez {

Processing::Processing(ImageSource *imgsrc, QObject *parent)
:	QObject(parent)
,	m_imageSource(imgsrc)
,	m_calculateHandTracker(false)
,	m_handTracker(false)
{
	m_thread = new QThread(QCoreApplication::instance()->thread());
	moveToThread(m_thread);

	connect(imgsrc, SIGNAL(frameReceived()), this, SLOT(process()), Qt::QueuedConnection);
	connect(this, SIGNAL(got_learnNew(int)), this, SLOT(on_learnNew(int)));
	connect(this, SIGNAL(got_train(int)), this, SLOT(on_train(int)));
	m_thread->start();
}


Processing::~Processing(void)
{
	qDebug("X1");

	m_thread->exit();
	// If thread does not terminate within 10s, terminate it
	if (!m_thread->wait(10000)) {
		m_thread->terminate();
		m_thread->wait();
	}
	m_thread->deleteLater();

	qDebug("X1 finished");
}


void Processing::process(bool secondarySource)
{
	QMutexLocker l(&m_processMutex);
	const ImageSource *src;
	if (secondarySource) {
		if (m_secondarySource) {
			src = m_secondarySource.data();
		} else {
			Q_ASSERT("secondary source not initialized");
		}
	} else {
		src = m_imageSource;
	}
	const cv::Mat &depth = m_imageSource->getDepthMat();
	const cv::Mat &rgb = m_imageSource->getColorMat();
	cv::Mat bgr;
	cv::cvtColor(rgb, bgr,cv::COLOR_RGB2BGR);

	Q_ASSERT(bgr.rows == depth.rows);
	Q_ASSERT(bgr.cols == depth.cols);

//	WindowManager::getInstance()->imShow("Original", bgr);

	m_handTracker.process(bgr, depth, m_imageSource->getSequence());
	const HandTracker::Data handTrackerData = m_handTracker.data();

	QString poseString = m_pose.categorize(handTrackerData.palmCenter(),
					  handTrackerData.palmRadius(),
					  handTrackerData.wrist(),
					  handTrackerData.fingertips());
	emit got_poseUpdated(poseString);
	ImageSourceArtificial::globalInstance()->setColorMat(bgr);

	if (secondarySource) {
		m_handTrackerTemporaryResult = m_handTracker.temporaryResult();
	}
	m_handTrackerResult = m_handTracker.data();
}

void Processing::learnNew(enum PoseRecognition::POSE poseId)
{
	emit got_learnNew(poseId);
}

void Processing::on_learnNew(int poseId)
{
	const HandTracker::Data data = m_handTracker.data();
	qDebug("LEARNING");
	qDebug("%d", poseId);
	qDebug("(%d, %d) R=%f", data.palmCenter().x, data.palmCenter().y, data.palmRadius());
	qDebug("(%d, %d) (%d, %d)", data.wrist().first.x,
								data.wrist().first.y,
								data.wrist().second.x,
								data.wrist().second.y);
	qDebug("fingertips:");
	foreach (cv::Point p, data.fingertips()) {
		qDebug("\t(%d, %d)", p.x, p.y);
	}

	m_pose.learnNew(static_cast<enum PoseRecognition::POSE>(poseId),
				  data.palmCenter(),
				  data.palmRadius(),
				  data.wrist(),
					data.fingertips());
}

void Processing::train(int hiddenCount)
{
	emit got_train(hiddenCount);
}

void Processing::on_train(int hiddenCount)
{
	try {
		m_pose.train(hiddenCount);
	} catch (std::logic_error e) {
		qDebug("%s", e.what());
		QApplication::quit();
	}

	emit got_trainingFinished();
}

PoseRecognition *Processing::pose()
{
	return &m_pose;
}

void Processing::setSecondarySource(ImageSource *secondarySource)
{
	m_secondarySource = secondarySource;
}

bool Processing::handTrackerTemporaryResult(HandTracker::TemporaryResult &temporaryResult) const
{
	if (m_handTracker.isDebug()) {
		temporaryResult = m_handTrackerTemporaryResult;
	}
	return m_handTracker.isDebug();
}

HandTracker::Data Processing::handTrackerData() const
{
	return m_handTrackerResult;
}

}
