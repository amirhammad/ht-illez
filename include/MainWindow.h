#pragma once

#include <QMainWindow>

class QComboBox;
class QTextEdit;
class QTableWidget;
class QSpinBox;
#include <QPointer>
#include <QDialog>

namespace iez {
class ImageSourceOpenNI;
class Processing;

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

	void buildNNTeachDialog();
	void teachDialogShow();
	void train();
	void loadPoseDatabaseToTable();

	bool m_paused;

	struct {
		QComboBox *classComboBox;
	} m_teachDialogProperties;
	QWidget *m_teachDialog;

	QTableWidget *m_databaseTable;
	QTextEdit *m_nnResultTextEdit;

signals:
	void got_pause(bool p);

public slots:

	void closeEvent(QCloseEvent *event);
	void keyEvent(QKeyEvent *event);

private slots:
	void on_poseTrainDialogAccepted(PoseTrainDialog::Result);
	void on_addButtonClicked();
	void on_cancelButtonClicked();
	void on_neuralNetworkSave();
	void on_neuralNetworkLoad();
	void on_init();
	void on_buildVideo();
	void on_buildProcessing();
};




}
