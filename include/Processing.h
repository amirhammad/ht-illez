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

#include <opencv2/opencv.hpp>
#include <list>
#include <string>
#include "main.h"
#include "ImageSourceFreenect.h"
#include "WindowManager.h"
#include "HandTracker.h"
#include "PoseRecognition.h"
#include <QPointer>
#include <QThread>

namespace iez {


class Processing : public QObject {
	Q_OBJECT
public:
	explicit Processing(ImageSource *imgsrc, QObject *parent = 0);
	~Processing(void);

	void poseDatabaseAppend(const int);
	void train(PoseRecognition::TrainArgs args);
	PoseRecognition *pose();
	void setSecondarySource(ImageSource *secondarySource);
	bool handTrackerTemporaryResult(HandTracker::TemporaryResult & temporaryResult) const;
	HandTracker::Data handTrackerData() const;

public slots:
	void process(bool secondarySource = false);

private slots:
	void on_poseDatabaseAppend(int poseId);
	void on_train(iez::PoseRecognition::TrainArgs args);
private:
	QThread *m_thread;

	HandTracker m_handTracker;
	const ImageSource *m_imageSource;
	QPointer<ImageSource> m_secondarySource;
	bool m_calculateHandTracker;

	PoseRecognition m_pose;

	HandTracker::Data m_handTrackerResult;
	HandTracker::TemporaryResult m_handTrackerTemporaryResult;
	QMutex m_processMutex;
signals:
	void got_poseUpdated(QVariantList);
	void got_learnNew(int);
	void got_train(iez::PoseRecognition::TrainArgs);
	void got_trainingFinished();
};

}




