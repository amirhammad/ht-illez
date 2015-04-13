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

namespace iez {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle("HT-illez: Main Window");

	QDockWidget *databaseWidget = new QDockWidget("database", this);
	addDockWidget(Qt::LeftDockWidgetArea, databaseWidget);
	m_databaseTextEdit = new QTextEdit(this);
	databaseWidget->setWidget(m_databaseTextEdit);

	QDockWidget *nnResultWidget = new QDockWidget("results", this);
	addDockWidget(Qt::BottomDockWidgetArea, nnResultWidget);
	m_nnResultTextEdit = new QTextEdit(this);
	nnResultWidget->setWidget(m_nnResultTextEdit);

	QMenuBar *menuBar = new QMenuBar(this);
	QMenu *fileMenu = new QMenu("&File",this);
	fileMenu->addAction(QIcon(), "quit", this, SLOT(deleteLater()));
	menuBar->addMenu(fileMenu);
	setMenuBar(menuBar);

//	addDockWidget(Qt::RightDockWidgetArea, new QDockWidget("X", this));
//	QDockWidget *dw = new QDockWidget("Y", this);
//	QLayout *layout = dw->layout();
//	layout->setContentsMargins(0, 20, 0, 0);
//	QLabel *label = new QLabel(tr("xokokok"));
//	label->show();
//	dw->setWidget(label);

//	addDockWidget(Qt::LeftDockWidgetArea, dw);
//	addDockWidget(Qt::LeftDockWidgetArea, new QDockWidget("Z", this));
//	QToolBar * t = new QToolBar(tr("HAHA"));
//	addToolBar(Qt::LeftToolBarArea, t);

//	QAction * action = t->addAction("helloasdfsdfasdfsadfsadfsadfasdf");
//	QMenu *menu = new QMenu(this);
//	menu->addAction(tr("x"));
//	action->setMenu(menu);
//	QObject::connect(action, SIGNAL(triggered()), this, SLOT(oh()));

	m_video = new iez::ImageSourceOpenNI(0);
	m_video->init();
	m_processing = new iez::Processing(m_video, 0);

	// Handle window manager key events
	QObject::connect(&WindowManager::getInstance(), SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(keyEvent(QKeyEvent*)));
	QObject::connect(this, SIGNAL(got_pause(bool)), m_video, SLOT(pause(bool)), Qt::QueuedConnection);

	QObject::connect(m_processing, SIGNAL(got_poseUpdated(QString)), m_nnResultTextEdit, SLOT(setText(QString)));

	m_paused = false;


	m_databaseTextEdit->setText(m_processing->poseDatabaseToString());

	buildNNTeachDialog();

	showMaximized();
}


MainWindow::~MainWindow()
{
	m_video->deleteLater();
	m_processing->deleteLater();
	m_teachDialog->deleteLater();
	WindowManager::destroy();
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
	emit got_pause(true);
	m_processing->train();
	emit got_pause(false);
}

void MainWindow::on_addButtonClicked()
{
	m_teachDialog->hide();

	m_processing->learnNew(static_cast<enum PoseRecognition::POSE>(m_teachDialogProperties.classComboBox->currentIndex()));

	m_databaseTextEdit->setText(m_processing->poseDatabaseToString());
	emit got_pause(false);
}

void MainWindow::on_cancelButtonClicked()
{
	m_teachDialog->hide();
	emit got_pause(false);
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

	case Qt::Key_S:
		m_processing->savePoseDatabase();
		break;
	default:
		break;

	}
}

}
