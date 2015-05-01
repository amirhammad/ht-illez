#pragma once

#include <opencv2/opencv.hpp>
#include <list>
#include <string>
#include "main.h"
#include "ImageSourceFreenect.h"
#include "WindowManager.h"
#include "HandTracker.h"
#include <QPointer>
#include <QThread>

namespace iez {


class Processing : public QObject {
	Q_OBJECT
public:
	explicit Processing(ImageSource *imgsrc, QObject *parent = 0);
	~Processing(void);

	void learnNew(enum PoseRecognition::POSE);
	void train(int hiddenCount);
	PoseRecognition *pose();
	void setSecondarySource(ImageSource *secondarySource);
	bool handTrackerTemporaryResult(HandTracker::TemporaryResult & temporaryResult) const;
	HandTracker::Data handTrackerData() const;

public slots:
	void process(bool secondarySource = false);

private slots:
	void on_learnNew(int poseId);
	void on_train(int hiddenCount);
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
	void got_poseUpdated(QString);
	void got_learnNew(int);
	void got_train(int);
	void got_trainingFinished();
};

}




