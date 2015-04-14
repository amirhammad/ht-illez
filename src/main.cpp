
// opencv
#include <opencv2/opencv.hpp>

// STL
#include <iostream>
#include <string>

// libfreenect
#include <libfreenect/libfreenect.hpp>

// Qt
//#include <QThread>
#include <QApplication>
#include <QKeyEvent>
// Custom
#include "ImageSource.h"
#include "ImageSourceFreenect.h"
#include "ImageSourceOpenNI.h"
#include "ImageRecorder.h"
#include "ColorSegmentation.h"
#include "ImageDescriptor.h"
#include "Processing.h"
#include "WindowManager.h"
#include "MainWindow.h"
#include "main.h"

#include "unistd.h"
#include "getopt.h"



iez::ImageSourceArtificial *iez::imageSourceArtificial;

static struct {
	const char * recordName;
	bool recording;
	bool playingRecord;
	bool artifical;

} options;

static void getOptions(int argc, char *argv[])
{
	using namespace std;
	options.recordName = 0;
	options.recording = false;
	options.playingRecord = false;
	options.artifical = false;
	static struct option long_options[] = {
			{"record", optional_argument, 0, 'r'},
			{"artifical", no_argument, 0, 'a'},
//			{"segment", optional_argument, 0, 'a'},
			{0, 0, 0, 0},
	};
	int index;
	while (1) {
		char c = getopt_long(argc, argv, "p:r:a", long_options, &index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 0:
			// long opts
			break;
		case 'r':
			if (options.playingRecord) {
				cerr << "playing record, cannot record" << endl;
				break;
			}
			options.recording = true;
			if (optarg) {
				options.recordName = optarg;
			} else {
				options.recordName = "_defaultOutput.oni";
			}
			break;
		case 'p':
			if (options.recording) {
				options.recording = false;
				cerr << "playing record, cannot record" << endl;
				break;
			}
			options.playingRecord = true;
			if (optarg) {
				options.recordName = optarg;
			} else {
				options.recordName = "_defaultInput.oni";
			}
			break;

		}
	}
}

int main(int argc, char *argv[])
{
//	using namespace iez;
//	using namespace std;
//	using namespace cv;
	QApplication app(argc, argv);
//	new MainWindow();
//	QApplication::exec();
//	return 0;
	getOptions(argc, argv);
	// Camera init
//	iez::ImageSourceFreenect *kinectFreenect = new iez::ImageSourceFreenect(0);

//	iez::ImageSourceOpenNI *kinect = new iez::ImageSourceOpenNI();

//	if (options.playingRecord) {
//		kinect->init(options.recordName);
//		if (!kinect->isInitialized()) {
//			cerr << "cannot open file" << options.recordName << endl;
//			return -1;
//		}
//	} else {
//		kinect->init();
//		if (!kinect->isInitialized()) {
//			cerr << "cannot open kinect" << endl;
//			return -1;
//		}
//	}

	//TEST
//	for (int i = 20; i < 100; i += 5) {
//		qWarning("\n#####");
//		QList<cv::Point> points = iez::HandTracker::findFingertip(
//					cv::RotatedRect(cv::Point2f(400, 400), cv::Size2f(i, 500), 0),
//					30,
//					cv::Point(300, 300));
//		foreach (cv::Point pt, points) {
//			qWarning("(%d %d) ", pt.x, pt.y);
//		}
//	}
//	return 0;
	iez::imageSourceArtificial = new iez::ImageSourceArtificial();
	new iez::MainWindow();

	// TODO: can edit files
//	iez::ImageRecorder *recorder;
//	if (options.recording) {
//		iez::ImageDescriptor *imageDescriptor = new iez::ImageDescriptor(kinect);
//		recorder = new iez::ImageRecorder();
//		recorder->init(kinect, options.recordName);
//	} else {
//		/**
//		 * Processing
//		 */
//		iez::Processing *processing = new iez::Processing(kinect);
//	}

//	iez::ColorSegmentation::buildDatabaseFromFiles("../database/colorDB_files.txt");

	QApplication::exec();
	return 0;
}

cv::Mat iez::ImageSourceArtificial::getColorMat() const
{
	QMutexLocker locker(&m_mutex);
	cv::Mat ret;
	m_color.copyTo(ret);
	return ret;
}

iez::ImageSourceArtificial::ImageSourceArtificial()
{
	QMutexLocker locker(&m_mutex);
	m_color.create(480, 640, CV_8UC3);
}

void iez::ImageSourceArtificial::setColorMat(const cv::Mat& src)
{
	QMutexLocker locker(&m_mutex);
	cvtColor(src, m_color, cv::COLOR_RGB2BGR);
	m_sequence++;
}

void iez::ImageSourceArtificial::pause(bool p)
{
	Q_UNUSED(p);
}



iez::Fps::Fps()
{
	m_timeLast = 0;
}

void iez::Fps::tick()
{
	m_timeLast = clock();
}

float iez::Fps::fps() const
{
	return 1000000.0f/(clock() - m_timeLast);
}
