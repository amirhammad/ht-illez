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

Processing::Processing(QObject *parent)
:	QObject(parent)
,	m_primaryImageSource(0)
,	m_secondaryImageSource(0)
,	m_calculateHandTracker(false)
,	m_handTracker(false)
{
	qRegisterMetaType<iez::PoseRecognition::TrainArgs>();

	m_thread = new QThread(QCoreApplication::instance()->thread());
	moveToThread(m_thread);

	connect(this, SIGNAL(got_learnNew(int)), this, SLOT(on_poseDatabaseAppend(int)));
	connect(this, SIGNAL(got_train(iez::PoseRecognition::TrainArgs)), this, SLOT(on_train(iez::PoseRecognition::TrainArgs)));
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
		if (m_secondaryImageSource) {
			src = m_secondaryImageSource.data();
		} else {
			Q_ASSERT("secondary source not initialized");
		}
	} else {
		src = m_primaryImageSource;
	}
	const cv::Mat &depth = src->getDepthMat();
	const cv::Mat &rgb = src->getColorMat();
	cv::Mat bgr;
	cv::cvtColor(rgb, bgr,cv::COLOR_RGB2BGR);

	Q_ASSERT(bgr.rows == depth.rows);
	Q_ASSERT(bgr.cols == depth.cols);

//	WindowManager::getInstance()->imShow("Original", bgr);

	m_handTracker.process(bgr, depth, m_primaryImageSource->getSequence());
	const HandTracker::Data handTrackerData = m_handTracker.data();

	QVariantList poseResultList = m_pose.categorize(handTrackerData.palmCenter(),
					  handTrackerData.palmRadius(),
					  handTrackerData.wrist(),
					  handTrackerData.fingertips());
	emit got_poseUpdated(poseResultList);
	ImageSourceArtificial::globalInstance()->setColorMat(bgr);

	if (secondarySource) {
		m_handTrackerTemporaryResult = m_handTracker.temporaryResult();

        // add fingertipsNormalized
        m_handTrackerTemporaryResult.fingertipsNormalized = PoseRecognition::constructFeatureQVector(handTrackerData.palmCenter(),
                                                                                                     handTrackerData.palmRadius(),
                                                                                                     handTrackerData.wrist(),
                                                                                                     handTrackerData.fingertips());
	}
	m_handTrackerResult = m_handTracker.data();
}

void Processing::poseDatabaseAppend(const int poseId)
{
	emit got_learnNew(poseId);
}

void Processing::on_poseDatabaseAppend(int poseId)
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

	m_pose.poseDatabaseAppend(poseId,
				  data.palmCenter(),
				  data.palmRadius(),
				  data.wrist(),
					data.fingertips());
}

void Processing::train(PoseRecognition::TrainArgs args)
{
	emit got_train(args);
}

void Processing::on_train(PoseRecognition::TrainArgs args)
{
	try {
		m_pose.neuralNetworkTrain(args);
	} catch (std::logic_error &e) {
		qDebug("%s", e.what());
		QApplication::quit();
	}

	emit got_trainingFinished();
}

PoseRecognition *Processing::pose()
{
	return &m_pose;
}

void Processing::setPrimaryImageSource(ImageSource *primaryImageSource)
{
	Q_ASSERT(primaryImageSource);

	m_primaryImageSource = primaryImageSource;
	connect(m_primaryImageSource, SIGNAL(frameReceived()), this, SLOT(process()), Qt::QueuedConnection);
}

void Processing::setSecondaryImageSource(ImageSource *secondaryImageSource)
{
	m_secondaryImageSource = secondaryImageSource;
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

Q_DECLARE_METATYPE(iez::PoseRecognition::TrainArgs)
