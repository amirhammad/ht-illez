
// opencv
#include <opencv2/opencv.hpp>

// STL
#include <iostream>
#include <string>

// armadillo
//#include <armadillo>

// libfreenect
#include <libfreenect/libfreenect.hpp>

// Qt
//#include <QThread>
#include <QApplication>
// Custom
#include "ImageSource.h"
#include "ImageSourceFreenect.h"
#include "ImageSourceOpenNI.h"
#include "ColorSegmentation.h"
#include "ImageDescriptor.h"
#include "Processing.h"
#include "WindowManager.h"
#include "main.h"

#include "unistd.h"
#include "getopt.h"
using namespace cv;


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
	using namespace iez;
	using namespace std;
	QApplication app(argc, argv);

	getOptions(argc, argv);
	// Camera init
//	iez::ImageSourceFreenect *kinectFreenect = new iez::ImageSourceFreenect(0);
	iez::ImageSourceBase *kinect = new iez::ImageSourceFreenect();

	/**
	 * Processing
	 */

	iez::imageSourceArtificial = new iez::ImageSourceArtificial();
	// TODO: can edit files
	iez::ImageDescriptor *imageDescriptor = new iez::ImageDescriptor(iez::imageSourceArtificial);

	iez::ColorSegmentation::buildDatabaseFromFiles("../database/colorDB_files.txt");

	iez::Processing *processing = new iez::Processing(kinect);

	return QApplication::exec();
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
