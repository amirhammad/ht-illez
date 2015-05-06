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


#include "MainWindow.h"

#include "main.h"
#include "WindowManager.h"
#include "PoseRecognition.h"
#include "Processing.h"
#include "ImageSource.h"
#include "ImageSourceArtificial.h"
#include "ImageSourceOpenNI.h"
#include "ImageRecorder.h"
#include "Util.h"

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
#include <QProgressBar>

namespace iez {

#define CHECK_PROCESSING() if (!m_processing) {QMessageBox::warning(this, "error", "Processing not initialized");return;}
#define CHECK_VIDEO() if (!m_video) {QMessageBox::warning(this, "error", "Video not initialized");return;}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
,	m_secondaryImageSource(new ImageSourceArtificial())
,	m_imageRecorder(new ImageRecorder())
{
	qRegisterMetaType<iez::PoseRecognition::TrainArgs>();

	PoseTrainDialog *poseTrainDialog = new PoseTrainDialog(this);
	connect(poseTrainDialog, SIGNAL(got_accepted(iez::PoseRecognition::TrainArgs)), this, SLOT(on_poseTrainDialogAccepted(iez::PoseRecognition::TrainArgs)));

	setWindowTitle("::: Hand Tracker - IEZ - Main Window :::");
	setMinimumWidth(400);
	m_databaseTable = new QTableWidget(this);

	m_neuralNetworkResultWidget = new NeuralNetworkResultWidget(this);

	QTabWidget *tabWidget = new QTabWidget(this);
	tabWidget->addTab(m_databaseTable, "Database");
	tabWidget->addTab(m_neuralNetworkResultWidget, "Results");
	tabWidget->setCurrentIndex(1);

	setCentralWidget(tabWidget);

	buildNNTeachDialog();

	QMenuBar *menuBar = new QMenuBar(this);

	QMenu *fileMenu = new QMenu("&File", this);
	fileMenu->addAction(QIcon(), "&Open record", this, SLOT(on_openRecord()), QKeySequence(Qt::CTRL + Qt::Key_O));
	fileMenu->addAction(QIcon(), "Open &camera", this, SLOT(on_buildVideo()), QKeySequence(Qt::ALT + Qt::Key_C));
	fileMenu->addAction(QIcon(), "Open &processing", this, SLOT(on_buildProcessing()), QKeySequence(Qt::ALT + Qt::Key_P));
	fileMenu->addSeparator();
	fileMenu->addAction(QIcon(), "Initialize recorder", this, SLOT(on_recorderAttach()));
	fileMenu->addSeparator();
	fileMenu->addAction(QIcon(), "&Quit", this, SLOT(deleteLater()), QKeySequence(Qt::ALT + Qt::Key_Q));

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

	// Handle window manager key events
	QObject::connect(WindowManager::getInstance(), SIGNAL(keyPressed(int)), this, SLOT(keyEvent(int)), Qt::QueuedConnection);

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

	if (m_imageRecorder) {
		delete m_imageRecorder;
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
	int index;

	if (!prefix.endsWith("/")) {
		prefix += "/";
	}

	QImage res;
	res = WindowManager::Mat2QImage(debugResult.result);
	res.save(prefix + "result.jpg", "jpg", 100);

	double max = Util::findMax(debugResult.distanceTransform);
	cv::Mat distanceTransformOutput;
	debugResult.distanceTransform.convertTo(distanceTransformOutput, CV_8UC1, 255.0/max, 0);
	res = WindowManager::Mat2QImage(distanceTransformOutput);
	res.save(prefix + "distanceTransform.jpg", "jpg", 100);

	index = 0;
	foreach (cv::Mat img, debugResult.medianList) {
		res = WindowManager::Mat2QImage(img);
		res.save(prefix + QString("medianFilter-%1.jpg").arg(index++), "jpg", 100);
	}

	res = WindowManager::Mat2QImage(debugResult.handMask);
	res.save(prefix + "handMask.jpg", "jpg", 100);

	res = WindowManager::Mat2QImage(debugResult.palmMask);
	res.save(prefix + "palmMask.jpg", "jpg", 100);

	res = WindowManager::Mat2QImage(debugResult.fingersMask);
	res.save(prefix + "fingersMask.jpg", "jpg", 100);

	res = WindowManager::Mat2QImage(debugResult.depthMaskedImage);
	res.save(prefix + "depthMaskedImage.jpg", "jpg", 100);

}

void MainWindow::on_gestureTrainerFinished(int code)
{
	CHECK_PROCESSING();

	switch (code) {
	case QDialog::Accepted:
		m_processing->learnNew(m_teachDialogProperties.classComboBox->currentIndex());

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

void MainWindow::on_recorderAttach()
{
	if (!m_video) {
		QMessageBox::warning(this, "Error", "video not ready");
		return;
	}

	QString path = QFileDialog::getSaveFileName(this, "Select output record file", QString(), "OpenNI Record (*.oni)");
	if (!path.isEmpty()) {
		if (!path.endsWith(".oni")) {
			path.append(".oni");
		}
		m_imageRecorder->init(m_video.data(), path);
	}

}

void MainWindow::on_openRecord()
{
	QString path = QFileDialog::getOpenFileName(this, "Select record", QString(), "OpenNI Record (*.oni)");
	if (!path.isEmpty()) {
		on_buildVideo(path);
	}
}

void MainWindow::on_buildVideo(QString path)
{
	if (m_video) {
		setStatusTip("Video device is already opened");
		return;
	}

	m_video = new iez::ImageSourceOpenNI();
	if (!m_video->init(path)) {
		delete m_video;
		if (path.isEmpty()) {
			QMessageBox::critical(this, QString("Cannot open camera device"), "Try reconnect the camera device or restart the application");
		} else {
			QMessageBox::critical(this, QString("Error opening record"), QString("Cannot open file %1. Try other record").arg(path));
		}
		return;
	}

	if (m_video) {
		QObject::connect(this, SIGNAL(got_pause(bool)), m_video, SLOT(pause(bool)), Qt::QueuedConnection);
	}
}

void MainWindow::on_buildProcessing()
{
	if (!m_video) {
		QMessageBox::information(this, "Error", "video not ready, using processing without image source");
	}

	if (m_processing) {
		QObject::disconnect(m_processing, SIGNAL(got_poseUpdated(QVariantList)), this, SLOT(on_poseUpdated(QVariantList)));
		delete m_processing;
	}

	if (m_video) {
		m_processing = new iez::Processing(m_video, 0);
	} else {
		m_processing = new iez::Processing(m_secondaryImageSource, 0);
	}
	loadPoseDatabaseToTable();
	QObject::connect(m_processing, SIGNAL(got_poseUpdated(QVariantList)), this, SLOT(on_poseUpdated(QVariantList)), Qt::QueuedConnection);
	QObject::connect(m_processing, SIGNAL(got_trainingFinished()), this, SLOT(on_trainingFinished()), Qt::QueuedConnection);
}

void MainWindow::on_exportProcessData()
{
	CHECK_PROCESSING();
	CHECK_VIDEO();

	m_processing->setSecondarySource(m_secondaryImageSource);
	m_secondaryImageSource->setColorMat(m_video->getColorMat());
	m_secondaryImageSource->setDepthMat(m_video->getDepthMat());

	m_processing->process(true);

	HandTracker::TemporaryResult tempResult;
	bool valid = m_processing->handTrackerTemporaryResult(tempResult);
	if (valid) {
		QString prefix = QFileDialog::getExistingDirectory(this, "Select directory for output debug files");
		if (prefix.isEmpty()) {
			return;
		}
		exportProcessData(prefix, m_processing->handTrackerData(), tempResult);
	} else {
		QMessageBox::warning(this, "Debug not active", "close, edit, recompile, run, try again. Good luck. \nHint: Check constructor of m_handTracker");
	}
}

void MainWindow::on_poseUpdated(QVariantList list)
{
	if (list.empty()) {
		return;
	}
	Q_ASSERT(list.size() == PoseRecognition::POSE_END + 1);
	QVector<float> neuronOutputs(PoseRecognition::POSE_END);
	for (int i = 0; i < neuronOutputs.size(); i++) {
		bool ok = true;
		neuronOutputs[i] = list[i].toFloat(&ok);
		Q_ASSERT(ok);
	}
	bool ok = true;
	int winner = list[PoseRecognition::POSE_END].toInt(&ok);
	Q_ASSERT(ok);
	m_neuralNetworkResultWidget->setNeurons(neuronOutputs, winner);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);
	deleteLater();
}

void MainWindow::keyEvent(int key)
{
	qDebug("%d pressed", key);
	int modifiers = key&Qt::KeyboardModifierMask;
	key &= ~Qt::KeyboardModifierMask;
	switch (key) {
	case Qt::Key_Space:
		qDebug("pause emited");
		emit got_pause(!m_paused);
		m_paused = !m_paused;
		break;

	case Qt::Key_Q:
		deleteLater();
		break;

	case Qt::Key_F5:
		loadPoseDatabaseToTable();
		break;

	case Qt::Key_A:
		if (modifiers & Qt::ControlModifier) {
			on_teachDialog();
		}
		break;

	default:
		break;

	}
}

void MainWindow::on_poseTrainDialogAccepted(PoseRecognition::TrainArgs result)
{
	qDebug("Neurons %d", result.hiddenNeuronCount);
	CHECK_PROCESSING();
	setStatusTip("Training database");
	emit got_pause(true);
	m_processing->train(result);
}

void MainWindow::on_poseDatabaseLoad()
{
	CHECK_PROCESSING();

	QString path = QFileDialog::getOpenFileName(this, QString("load pose database"), QString(), QString("*.csv"));
	if (!path.isEmpty()) {
		m_processing->pose()->poseDatabaseLoad(path);
	}
	loadPoseDatabaseToTable();
}

void MainWindow::on_poseDatabaseSave()
{
	CHECK_PROCESSING();

	QString path = QFileDialog::getSaveFileName(this, QString("save pose database"), QString(), QString("*.csv"));
	if (!path.endsWith(".csv")) path.append(".csv");
	if (!path.isEmpty()) {
		m_processing->pose()->poseDatabaseSave(path);
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

	int row = 0;
	// First row
	QGridLayout *layout = new QGridLayout();
	QLabel *label = new QLabel("Number of hidden neurons", this);
	label->setAlignment(Qt::AlignRight);
	m_spinBox = new QSpinBox(this);
	m_spinBox->setValue(5);
	layout->addWidget(label, row, 0);
	layout->addWidget(m_spinBox, row, 1);
	mainLayout->addLayout(layout);
	row++;

	// Second row
	layout = new QGridLayout();
	label = new QLabel("Activation function", this);
	label->setAlignment(Qt::AlignRight);
	m_comboBox = new QComboBox(this);
	m_comboBox->addItem("Hyperbolic Tangent");
	m_comboBox->addItem("Logistic Sigmoid");
	m_comboBox->addItem("Linear");
	layout->addWidget(label, row, 0);
	layout->addWidget(m_comboBox, row, 1);
	mainLayout->addLayout(layout);
	row++;

	// last row
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	QPushButton *acceptButton = new QPushButton("accept", this);
	QPushButton *cancelButton = new QPushButton("cancel", this);

	buttonLayout->addWidget(acceptButton);
	buttonLayout->addWidget(cancelButton);
	mainLayout->addLayout(buttonLayout);

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(this, SIGNAL(accepted()), this, SLOT(on_accepted()));
}

PoseTrainDialog::~PoseTrainDialog()
{

}

void PoseTrainDialog::on_accepted()
{
	PoseRecognition::TrainArgs result;
	result.hiddenNeuronCount = m_spinBox->value();

	const int index = m_comboBox->currentIndex();
	PoseRecognition::TrainArgs::ActivationFunction function;
	switch (index) {
	case 0:
		function = PoseRecognition::TrainArgs::HYPERBOLIC_TANGENT;
		break;

	case 1:
		function = PoseRecognition::TrainArgs::LOGISTIC_SIGMOID;
		break;

	case 2:
		function = PoseRecognition::TrainArgs::LINEAR;
		break;


	default:
		Q_ASSERT(0);
		break;
	}

	result.activationFunction = function;
	emit got_accepted(result);
}

NeuralNetworkResultWidget::NeuralNetworkResultWidget(QWidget *parent)
	: QWidget(parent)
{
	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	QGridLayout *neuronLayout = new QGridLayout();
	m_neuronVector.resize(PoseRecognition::POSE_END);
	for (int i = 0; i < PoseRecognition::POSE_END; i++) {
		m_neuronVector[i] = new QProgressBar(this);
		m_neuronVector[i]->setMinimum(0);
		m_neuronVector[i]->setMaximum(100);
		m_neuronVector[i]->setTextVisible(false);
		QLabel *label = new QLabel(QString::number(i), this);
		label->setAlignment(Qt::AlignRight);
		neuronLayout->addWidget(label, i, 0);
		neuronLayout->addWidget(m_neuronVector[i], i, 1);
	}
	mainLayout->addLayout(neuronLayout);

	m_winnerLabel = new QLabel(this);
	m_winnerLabel->setText("--");
	m_winnerLabel->setMinimumWidth(100);
	QFont f = m_winnerLabel->font();
	f.setPointSize(50);
	m_winnerLabel->setFont(f);
	mainLayout->addWidget(m_winnerLabel);
}

NeuralNetworkResultWidget::~NeuralNetworkResultWidget()
{

}

void NeuralNetworkResultWidget::setNeurons(QVector<float> neurons, int winner)
{
	Q_ASSERT(neurons.size() == PoseRecognition::POSE_END);
	for (int i = 0; i < PoseRecognition::POSE_END; i++) {
		m_neuronVector[i]->setValue(scaleNeuronToInt(neurons[i]));
	}
	m_winnerLabel->setText(QString::number(winner));
}

int NeuralNetworkResultWidget::scaleNeuronToInt(float neuronValue)
{
	if (neuronValue < 0) {
		return MIN;
	} else if (neuronValue > 1) {
		return MAX;
	}
	return neuronValue*(MAX - MIN);
}


}

Q_DECLARE_METATYPE(iez::PoseRecognition::TrainArgs)
