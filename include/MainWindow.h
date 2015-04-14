#pragma once

#include <QMainWindow>
#include <QComboBox>
class QTextEdit;
namespace iez {
class ImageSourceOpenNI;
class Processing;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	virtual ~MainWindow();

private:
	ImageSourceOpenNI *m_video;
	Processing *m_processing;

	void buildNNTeachDialog();
	void teachDialogShow();
	void train();

	bool m_paused;

	struct {
		QComboBox *classComboBox;
	} m_teachDialogProperties;
	QWidget *m_teachDialog;

	QTextEdit *m_databaseTextEdit;
	QTextEdit *m_nnResultTextEdit;

signals:
	void got_pause(bool p);

public slots:

	void closeEvent(QCloseEvent *event);
	void keyEvent(QKeyEvent *event);

	void on_addButtonClicked();
	void on_cancelButtonClicked();
	void on_neuralNetworkSave();
	void on_neuralNetworkLoad();
};

}
