#include "MainWindow.h"

#include "WindowManager.h"
#include "PoseRecognition.h"
#include "Processing.h"
#include "ImageSource.h"
#include "ImageSourceOpenNI.h"

#include <QToolBar>
#include <QDebug>
#include <QObject>
#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QMenuBar>
#include <QTableWidget>
#include <QPointer>
#include <QMessageBox>
#include <QSpinBox>
#include <QInputDialog>

namespace iez {
//#define PATH_TO_VIDEO "/home/amir/git/amirhammad/diplomovka/ht-illez/build/_record001.oni"
#define PATH_TO_VIDEO
#define NEURAL_NETWORK_FILENAME "NeuralNet.dat"


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	qRegisterMetaType<PoseTrainDialog::Result>();
	PoseTrainDialog *poseTrainDialog = new PoseTrainDialog(this);
	connect(poseTrainDialog, SIGNAL(got_accepted(PoseTrainDialog::Result)), this, SLOT(on_poseTrainDialogAccepted(PoseTrainDialog::Result)));

	setWindowTitle("::: Hand Tracker - IEZ - Main Window :::");
	QDockWidget *databaseWidget = new QDockWidget("database", this);
	addDockWidget(Qt::TopDockWidgetArea, databaseWidget);
	m_databaseTable = new QTableWidget(this);
	databaseWidget->setWidget(m_databaseTable);

	QDockWidget *nnResultWidget = new QDockWidget("results", this);
	addDockWidget(Qt::TopDockWidgetArea, nnResultWidget);
	m_nnResultTextEdit = new QTextEdit(this);
	nnResultWidget->setWidget(m_nnResultTextEdit);

	QMenuBar *menuBar = new QMenuBar(this);

	QMenu *fileMenu = new QMenu("&File", this);
	fileMenu->addAction(QIcon(), "Open &Camera", this, SLOT(on_buildVideo()));
	fileMenu->addAction(QIcon(), "Open &Processing", this, SLOT(on_buildProcessing()));
	fileMenu->addAction(QIcon(), "&Quit", this, SLOT(deleteLater()));

	menuBar->addMenu(fileMenu);

	QMenu *neuralNetworkMenu = new QMenu("NeuralNet", this);
	neuralNetworkMenu->addAction(QIcon(), "train", poseTrainDialog, SLOT(show()));
	neuralNetworkMenu->addSeparator();
	neuralNetworkMenu->addAction(QIcon(), "load", this, SLOT(on_neuralNetworkLoad()));
	neuralNetworkMenu->addAction(QIcon(), "save", this, SLOT(on_neuralNetworkSave()));


	menuBar->addMenu(neuralNetworkMenu);

	setMenuBar(menuBar);

	QMetaObject::invokeMethod(this, "on_init");

	// Handle window manager key events
	QObject::connect(WindowManager::getInstance(), SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(keyEvent(QKeyEvent*)), Qt::QueuedConnection);

	m_paused = false;


	buildNNTeachDialog();

	showMaximized();
}


MainWindow::~MainWindow()
{
	if (m_processing) {
		delete m_processing;
	}
	if (m_video) {
		delete m_video;
	}

	m_teachDialog->deleteLater();

	WindowManager::destroy();
	qDebug("Main window destroyed");
}

void MainWindow::buildNNTeachDialog()
{
	m_teachDialog = new QWidget();
	QWidget *dialog = m_teachDialog;
	QVBoxLayout *layout = new QVBoxLayout(dialog);
	QLabel *textLabel = new QLabel("choose one of the class for this gesture");
	layout->addWidget(textLabel);

	QComboBox *classComboBox = new QComboBox(dialog);
	for (int i = 0; i < PoseRecognition::POSE_END; i++) {
		classComboBox->addItem(PoseRecognition::poseToString(i));
	}
	layout->addWidget(classComboBox);
	m_teachDialogProperties.classComboBox = classComboBox;

	QHBoxLayout *layoutButtons = new QHBoxLayout;
	QPushButton *addButton = new QPushButton("add to database", dialog);
	QPushButton *cancelButton = new QPushButton("cancel", dialog);
	layoutButtons->addWidget(addButton);
	layoutButtons->addWidget(cancelButton);
	layout->addLayout(layoutButtons);

	dialog->setLayout(layout);
	dialog->setWindowTitle("Gesture trainer");

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(on_cancelButtonClicked()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(on_addButtonClicked()));
}

void MainWindow::teachDialogShow()
{
	emit got_pause(true);
	m_teachDialog->show();
}

void MainWindow::train()
{
	if (!m_processing)
		return;

	emit got_pause(true);
	m_processing->train();
}

void MainWindow::loadPoseDatabaseToTable()
{
	if (!m_processing)
		return;

	QString poseDB = m_processing->poseDatabaseToString();
	QStringList poseDBLineList = poseDB.split('\n', QString::SkipEmptyParts);
	m_databaseTable->setRowCount(poseDBLineList.count());
	m_databaseTable->setColumnCount(PoseRecognition::inputVectorSize() + 1);
	int indexLine = 0;
	foreach (QString line, poseDBLineList) {
		QStringList inputOutputStringList = line.split("||", QString::SkipEmptyParts);
		QStringList inputStringList = inputOutputStringList[0].split(' ', QString::SkipEmptyParts);
		int indexColumn = 0;
		Q_ASSERT(inputStringList.size() == PoseRecognition::inputVectorSize());
		foreach (QString inputItem, inputStringList) {
			QTableWidgetItem *tableWidgetItem = new QTableWidgetItem(inputItem);
			m_databaseTable->setItem(indexLine, indexColumn++, tableWidgetItem);
		}

		QString outputItem = inputOutputStringList[1];
		QTableWidgetItem *tableWidgetItem = new QTableWidgetItem(outputItem);
		m_databaseTable->setItem(indexLine, PoseRecognition::inputVectorSize(), tableWidgetItem);
		indexLine++;
	}


}

void MainWindow::on_addButtonClicked()
{
	if (!m_processing)
		return;

	m_teachDialog->hide();

	m_processing->learnNew(static_cast<enum PoseRecognition::POSE>(m_teachDialogProperties.classComboBox->currentIndex()));

	emit got_pause(false);
}

void MainWindow::on_cancelButtonClicked()
{
	if (!m_processing)
		return;

	m_teachDialog->hide();
	emit got_pause(false);
}

void MainWindow::on_neuralNetworkSave()
{
	if (!m_processing)
		return;

	m_processing->neuralNetworkSave(NEURAL_NETWORK_FILENAME);
}

void MainWindow::on_neuralNetworkLoad()
{
	if (!m_processing)
		return;

	m_processing->neuralNetworkLoad(NEURAL_NETWORK_FILENAME);
}

void MainWindow::on_init()
{

}

void MainWindow::on_buildVideo()
{
	if (m_video) {
		return;
	}

	m_video = new iez::ImageSourceOpenNI();
	if (!m_video->init(PATH_TO_VIDEO)) {
		delete m_video;
		QMessageBox::critical(this, "Cannot open camera device", "Try reconnect the camera device or restart the application");
	}

	if (m_video) {
		QObject::connect(this, SIGNAL(got_pause(bool)), m_video, SLOT(pause(bool)), Qt::QueuedConnection);
	}
}

void MainWindow::on_buildProcessing()
{
	if (!m_video) {
		qDebug("Error: video not ready");
		return;
	}

	if (m_processing) {
		QObject::disconnect(m_processing, SIGNAL(got_poseUpdated(QString)), m_nnResultTextEdit, SLOT(setText(QString)));
		delete m_processing;
	}

	m_processing = new iez::Processing(m_video, 0);
	loadPoseDatabaseToTable();
	QObject::connect(m_processing, SIGNAL(got_poseUpdated(QString)), m_nnResultTextEdit, SLOT(setText(QString)), Qt::QueuedConnection);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);
	deleteLater();
}

void MainWindow::keyEvent(QKeyEvent *event)
{

	int key = event->key();
	qDebug("%d pressed", key);
	switch (key) {
	case Qt::Key_Space:
		qDebug("pause emited");
		emit got_pause(!m_paused);
		m_paused = !m_paused;
		break;

	case Qt::Key_Tab:
		teachDialogShow();
		break;

	case Qt::Key_T:
		train();
		break;

	case Qt::Key_Q:
		deleteLater();
		break;

	case Qt::Key_F5:
		loadPoseDatabaseToTable();
		break;

	case Qt::Key_S:
		if (!m_processing)
			return;

		m_processing->savePoseDatabase();
		break;

	case Qt::Key_P:
		if (!m_processing)
			return;

		delete m_processing;
		break;

	default:
		break;

	}
}

void MainWindow::on_poseTrainDialogAccepted(PoseTrainDialog::Result result)
{
	qDebug("Neurons %d", result.hiddenNeurons);
}

PoseTrainDialog::PoseTrainDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle("Train NN");
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	// Forms
	// First row
	QHBoxLayout *layout = new QHBoxLayout();
	QLabel *label = new QLabel("Number of hidden neurons", this);
	label->setAlignment(Qt::AlignRight);
	m_spinBox = new QSpinBox(this);
	m_spinBox->setValue(5);
	layout->addWidget(label);
	layout->addWidget(m_spinBox);
	mainLayout->addLayout(layout);

	// First row
	layout = new QHBoxLayout();
	QPushButton *button = new QPushButton("accept", this);

	layout->addWidget(button);
	mainLayout->addLayout(layout);

	connect(button, SIGNAL(clicked()), this, SLOT(accept()));
	connect(this, SIGNAL(accepted()), this, SLOT(on_accepted()));
}

PoseTrainDialog::~PoseTrainDialog()
{

}

void PoseTrainDialog::on_accepted()
{
	Result result;
	result.hiddenNeurons = m_spinBox->value();
	emit got_accepted(result);
}


}

Q_DECLARE_METATYPE(iez::PoseTrainDialog::Result)
