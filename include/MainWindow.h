#pragma once

#include "HandTracker.h"
#include <QMainWindow>

class QComboBox;
class QTextEdit;
class QTableWidget;
class QSpinBox;
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

	struct Result {
		int hiddenNeurons;
	};

signals:
	void got_accepted(PoseTrainDialog::Result);

private slots:
	void on_accepted();

private:
	QSpinBox *m_spinBox;
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
	void keyEvent(QKeyEvent *event);

private slots:
	void on_poseTrainDialogAccepted(PoseTrainDialog::Result);
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
