#include "ImageSourceFreenect_private.h"
#include "ImageSourceFreenect.h"
//using namespace iez_private;
using namespace cv;

#define SAMPLE_XML_PATH "SamplesConfig.xml"

iez_private::ImageSourceFreenectDevice_private::ImageSourceFreenectDevice_private(freenect_context *_ctx, int _index)
:	FreenectDevice(_ctx, _index)
,	m_fps(30)
,	m_sequence(0)
,	m_initialized(false)
,	m_width(0)
,	m_height(0)
{

}


iez_private::ImageSourceFreenectDevice_private::~ImageSourceFreenectDevice_private(void)
{
	stopVideo();

	stopDepth();
	setLed(LED_OFF);
}

int iez_private::ImageSourceFreenectDevice_private::deviceInit(void)
{

    return 0;
}

int iez_private::ImageSourceFreenectDevice_private::streamInit(freenect_resolution resolution)
{
	m_initialized = false;

	setVideoFormat(FREENECT_VIDEO_RGB, resolution);
	freenect_resolution reso = getVideoResolution();
	switch (reso) {
	case FREENECT_RESOLUTION_HIGH:
		m_width = 1280;
		m_height = 1024;
		break;
	case FREENECT_RESOLUTION_MEDIUM:
		m_width = 640;
		m_height = 480;
		break;
	default:
		qDebug("OTHER RESOLUTION");
	}

	m_depthMat.create(m_height, m_width, CV_16UC1);
	m_colorMat.create(m_height, m_width, CV_8UC3);
	startVideo();

	setDepthFormat(FREENECT_DEPTH_REGISTERED, resolution);

	startDepth();
	setFlag(FREENECT_RAW_COLOR, FREENECT_ON);
//	setFlag(FREENECT_AUTO_EXPOSURE, FREENECT_OFF);
	setFlag(FREENECT_AUTO_WHITE_BALANCE, FREENECT_OFF);
	setLed(LED_GREEN);
    return 0;
}


bool iez_private::ImageSourceFreenectDevice_private::isInitialized()
{
	return m_initialized;
}

cv::Mat iez_private::ImageSourceFreenectDevice_private::getDepthMat() const
{
	QMutexLocker locker(&depth_mutex);
	Mat ret;
	m_depthMat.copyTo(ret);
	return ret;
}


cv::Mat iez_private::ImageSourceFreenectDevice_private::getColorMat() const
{
	QMutexLocker locker(&color_mutex);
	cv::Mat ret;
	m_colorMat.copyTo(ret);
	return ret;
}

cv::Mat iez::ImageSourceFreenect::getColorMat() const
{
	return device->getColorMat();
}

cv::Mat iez::ImageSourceFreenect::getDepthMat() const
{
	return device->getDepthMat();
}

iez::ImageSourceFreenect::ImageSourceFreenect(const int index)
{
	device = &freenect.createDevice<iez_private::ImageSourceFreenectDevice_private>(index);
	streamInit(FREENECT_RESOLUTION_MEDIUM);
}

void iez::ImageSourceFreenect::streamInit(freenect_resolution resolution)
{
	device->streamInit(resolution);
}


