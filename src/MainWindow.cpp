#include "MainWindow.h"

#include "main.h"
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
#include <QFileDialog>
#include <QStatusBar>

namespace iez {
//#define PATH_TO_VIDEO "/home/amir/git/amirhammad/diplomovka/ht-illez/build/_record001.oni"
#define PATH_TO_VIDEO

#define CHECK_PROCESSING() if (!m_processing) {QMessageBox::warning(this, "error", "Processing not initialized");return;}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
,	m_secondaryImageSource(new ImageSourceArtificial())
{
	qRegisterMetaType<PoseTrainDialog::Result>();
	PoseTrainDialog *poseTrainDialog = new PoseTrainDialog(this);
	connect(poseTrainDialog, SIGNAL(got_accepted(PoseTrainDialog::Result)), this, SLOT(on_poseTrainDialogAccepted(PoseTrainDialog::Result)));

	setWindowTitle("::: Hand Tracker - IEZ - Main Window :::");
	QDockWidget *databaseWidget = new QDockWidget("database", this);
	addDockWidget(Qt::TopDockWidgetArea, databaseWidget);
	databaseWidget->setAcceptDrops(false);
	m_databaseTable = new QTableWidget(this);
	databaseWidget->setWidget(m_databaseTable);

	QDockWidget *nnResultWidget = new QDockWidget("results", this);
	addDockWidget(Qt::TopDockWidgetArea, nnResultWidget);
	m_nnResultTextEdit = new QTextEdit(this);
	m_nnResultTextEdit->setReadOnly(true);
	nnResultWidget->setWidget(m_nnResultTextEdit);

	buildNNTeachDialog();

	QMenuBar *menuBar = new QMenuBar(this);

	QMenu *fileMenu = new QMenu("&File", this);
	fileMenu->addAction(QIcon(), "Open &camera", this, SLOT(on_buildVideo()), QKeySequence::mnemonic("Open &camera"));
	fileMenu->addAction(QIcon(), "Open &processing", this, SLOT(on_buildProcessing()), QKeySequence(Qt::ALT + Qt::Key_P));
	fileMenu->addAction(QIcon(), "&Quit", this, SLOT(deleteLater()), QKeySequence::mnemonic("&Quit"));

	menuBar->addMenu(fileMenu);

	QMenu *neuralNetworkMenu = new QMenu("Neural Network", this);
	neuralNetworkMenu->addAction(QIcon(), "train", poseTrainDialog, SLOT(show()), QKeySequence(Qt::CTRL + Qt::Key_T));
	neuralNetworkMenu->addSeparator();
	neuralNetworkMenu->addAction(QIcon(), "import", this, SLOT(on_neuralNetworkImport()));
	neuralNetworkMenu->addAction(QIcon(), "load", this, SLOT(on_neuralNetworkLoad()));
	neuralNetworkMenu->addAction(QIcon(), "save", this, SLOT(on_neuralNetworkSave()));

	menuBar->addMenu(neuralNetworkMenu);

	QMenu *poseDatabaseMenu = new QMenu("Pose Database", this);
	poseDatabaseMenu->addAction(QIcon(), "add to database", this, SLOT(on_teachDialog()), QKeySequence(Qt::CTRL + Qt::Key_A));
	poseDatabaseMenu->addSeparator();
	poseDatabaseMenu->addAction(QIcon(), "load", this, SLOT(on_poseDatabaseLoad()));
	poseDatabaseMenu->addAction(QIcon(), "save", this, SLOT(on_poseDatabaseSave()));

	menuBar->addMenu(poseDatabaseMenu);

	QMenu *debugMenu = new QMenu("Debug", this);
	debugMenu->addAction(QIcon(), "export process data", this, SLOT(on_exportProcessData()));

	menuBar->addMenu(debugMenu);

	setMenuBar(menuBar);

	m_statusBar = new QStatusBar(this);
	setStatusBar(m_statusBar);

	QMetaObject::invokeMethod(this, "on_init");

	// Handle window manager key events
	QObject::connect(WindowManager::getInstance(), SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(keyEvent(QKeyEvent*)), Qt::QueuedConnection);

	m_paused = false;

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
	QDialog *dialog = new QDialog(this);
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
	addButton->setDefault(true);
	QPushButton *cancelButton = new QPushButton("cancel", dialog);
	layoutButtons->addWidget(addButton);
	layoutButtons->addWidget(cancelButton);
	layout->addLayout(layoutButtons);

	dialog->setLayout(layout);
	dialog->setWindowTitle("Gesture trainer");

	connect(cancelButton, SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(addButton, SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(dialog, SIGNAL(finished(int)), this, SLOT(on_gestureTrainerFinished(int)));

	m_teachDialog = dialog;
}

void MainWindow::train()
{
	CHECK_PROCESSING();
	setStatusTip("Training database");
	emit got_pause(true);
	m_processing->train();
}

void MainWindow::loadPoseDatabaseToTable()
{
	CHECK_PROCESSING();


	QString poseDB = m_processing->pose()->databaseToString();
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
			tableWidgetItem->setFlags(Qt::ItemIsSelectable);
			m_databaseTable->setItem(indexLine, indexColumn++, tableWidgetItem);
		}

		QString outputItem = inputOutputStringList[1];
		QTableWidgetItem *tableWidgetItem = new QTableWidgetItem(outputItem);
		tableWidgetItem->setFlags(Qt::ItemIsSelectable);
		m_databaseTable->setItem(indexLine, PoseRecognition::inputVectorSize(), tableWidgetItem);
		indexLine++;
	}


}

void MainWindow::exportProcessData(QString prefix, HandTracker::Data result, HandTracker::TemporaryResult debugResult)
{
	if (!prefix.endsWith("/")) {
		prefix += "/";
	}

	QImage res = WindowManager::Mat2QImage(debugResult.result);
	res.save(prefix + "/result.jpg", "jpg", 100);
}
//void MainWindow::on_addButtonClicked()
//{
//	if (!m_processing)
//		return;

//	m_teachDialog->hide();

//	m_processing->learnNew(static_cast<enum PoseRecognition::POSE>(m_teachDialogProperties.classComboBox->currentIndex()));

//	emit got_pause(false);
//}

//void MainWindow::on_cancelButtonClicked()
//{
//	if (!m_processing)
//		return;

//	m_teachDialog->hide();
//	emit got_pause(false);
//}

void MainWindow::on_gestureTrainerFinished(int code)
{
	switch (code) {
	case QDialog::Accepted:
		if (m_processing) {
			m_processing->learnNew(static_cast<enum PoseRecognition::POSE>(m_teachDialogProperties.classComboBox->currentIndex()));
		}
		emit got_pause(false);
		break;

	case QDialog::Rejected:
		emit got_pause(false);
		break;

	default:
		Q_ASSERT(0);
		break;
	}
}

void MainWindow::on_trainingFinished()
{
	setStatusTip("Training finished");
}

void MainWindow::on_neuralNetworkSave()
{
	CHECK_PROCESSING();

	QString path = QFileDialog::getSaveFileName(this, QString("save neural network"), QString(), QString("*.nndb"));
	if (!path.endsWith(".nndb")) path.append(".nndb");
	if (!path.isEmpty()) {
		m_processing->pose()->neuralNetworkSave(path);
	}
}

void MainWindow::on_neuralNetworkLoad()
{
	CHECK_PROCESSING();

	QString path = QFileDialog::getOpenFileName(this, QString("load neural network"), QString(), QString("*.nndb"));
	if (!path.isEmpty()) {
		m_processing->pose()->neuralNetworkLoad(path);
	}
}

void MainWindow::on_neuralNetworkImport()
{
	CHECK_PROCESSING();

	QString path = QFileDialog::getExistingDirectory(this, QString("Import neural network"), QString(""));
	if (!path.isEmpty()) {
		m_processing->pose()->neuralNetworkImport(path);
	}
}

void MainWindow::on_init()
{

}

void MainWindow::on_buildVideo()
{
	if (m_video) {
		setStatusTip("Video device is already opened");
		return;
	}

	m_video = new iez::ImageSourceOpenNI();
	if (!m_video->init(PATH_TO_VIDEO)) {
		delete m_video;
		QMessageBox::critical(this, "Cannot open camera device", "Try reconnect the camera device or restart the application");
		return;
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
	QObject::connect(m_processing, SIGNAL(got_trainingFinished()), this, SLOT(on_trainingFinished()), Qt::QueuedConnection);
}

void MainWindow::on_exportProcessData()
{
	CHECK_PROCESSING();

	m_processing->setSecondarySource(m_secondaryImageSource);
	m_secondaryImageSource->setColorMat(m_video->getColorMat());
	m_secondaryImageSource->setDepthMat(m_video->getDepthMat());

	m_processing->process(true);
	QString prefix = QFileDialog::getExistingDirectory(this, "Select directory for output debug files");
	if (prefix.isEmpty()) {
		return;
	}
	exportProcessData(prefix, m_processing->handTrackerData(), m_processing->handTrackerTemporaryResult());
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

	case Qt::Key_T:
		train();
		break;

	case Qt::Key_Q:
		deleteLater();
		break;

	case Qt::Key_F5:
		loadPoseDatabaseToTable();
		break;

	default:
		break;

	}
}

void MainWindow::on_poseTrainDialogAccepted(PoseTrainDialog::Result result)
{
	qDebug("Neurons %d", result.hiddenNeurons);
	train();
}

void MainWindow::on_poseDatabaseLoad()
{
	CHECK_PROCESSING();

	QString path = QFileDialog::getOpenFileName(this, QString("load pose database"), QString(), QString("*.csv"));
	if (!path.isEmpty()) {
		m_processing->pose()->loadPoseDatabase(path);
	}
}

void MainWindow::on_poseDatabaseSave()
{
	CHECK_PROCESSING();

	QString path = QFileDialog::getSaveFileName(this, QString("save pose database"), QString(), QString("*.csv"));
	if (!path.endsWith(".csv")) path.append(".csv");
	if (!path.isEmpty()) {
		m_processing->pose()->savePoseDatabase(path);
	}
}

void MainWindow::on_teachDialog()
{
	emit got_pause(true);
	m_teachDialog->show();
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
	QPushButton *acceptButton = new QPushButton("accept", this);
	QPushButton *cancelButton = new QPushButton("cancel", this);

	layout->addWidget(acceptButton);
	layout->addWidget(cancelButton);
	mainLayout->addLayout(layout);

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
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
