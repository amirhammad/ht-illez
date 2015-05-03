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

#include "HandTracker.h"
#include "PoseRecognition.h"
#include <QMainWindow>

class QComboBox;
class QTextEdit;
class QTableWidget;
class QSpinBox;
class QComboBox;
#include <QPointer>
#include <QDialog>

namespace iez {
class ImageSourceOpenNI;
class ImageSourceArtificial;
class Processing;
class ImageRecorder;

class PoseTrainDialog : public QDialog {
	Q_OBJECT
public:
	PoseTrainDialog(QWidget *parent = 0);
	virtual ~PoseTrainDialog();

signals:
	void got_accepted(iez::PoseRecognition::TrainArgs);

private slots:
	void on_accepted();

private:
	QSpinBox *m_spinBox;
	QComboBox *m_comboBox;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	virtual ~MainWindow();

private:

	QPointer<ImageSourceOpenNI> m_video;
	QPointer<Processing> m_processing;
	ImageRecorder *m_imageRecorder;

	void buildNNTeachDialog();
	void loadPoseDatabaseToTable();
	void exportProcessData(QString prefix, HandTracker::Data result, HandTracker::TemporaryResult debugResult);

	bool m_paused;

	struct {
		QComboBox *classComboBox;
	} m_teachDialogProperties;
	QWidget *m_teachDialog;

	QTableWidget *m_databaseTable;
	QTextEdit *m_nnResultTextEdit;
	QStatusBar *m_statusBar;

	ImageSourceArtificial *m_secondaryImageSource;

signals:
	void got_pause(bool p);

public slots:

	void closeEvent(QCloseEvent *event);
	void keyEvent(int);

private slots:
	void on_poseTrainDialogAccepted(iez::PoseRecognition::TrainArgs);
	void on_poseDatabaseLoad();
	void on_poseDatabaseSave();
	void on_teachDialog();
	void on_gestureTrainerFinished(int);
	void on_trainingFinished();
	void on_neuralNetworkSave();
	void on_neuralNetworkLoad();
	void on_neuralNetworkImport();
	void on_recorderAttach();
	void on_openRecord();
	void on_buildVideo(QString path = QString());
	void on_buildProcessing();
	void on_exportProcessData();

};




}
