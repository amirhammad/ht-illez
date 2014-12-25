#include "ImageSource.h"

using namespace iez;
using namespace cv;

#define SAMPLE_XML_PATH "SamplesConfig.xml"

ImageSource::ImageSource(int fps)
:	m_width(640)
,	m_height(480)
,	m_fps(fps)
,	m_sequence(0)
,	m_initialized(false)
{

}


ImageSource::~ImageSource(void)
{
	m_depthStream.stop();
	m_colorStream.stop();
	terminate();
	openni::OpenNI::shutdown();
}

int ImageSource::deviceInit(void)
{
	openni::Status rc = openni::STATUS_OK;

    const char* deviceURI = openni::ANY_DEVICE;

    rc = openni::OpenNI::initialize();

	rc = device.open(deviceURI);
    if (rc != openni::STATUS_OK)
    {
        printf("SimpleViewer: Device open failed:\n%s\n", openni::OpenNI::getExtendedError());
        openni::OpenNI::shutdown();
        return openni::STATUS_ERROR;
    }

    rc = m_depthStream.create(device, openni::SENSOR_DEPTH);
    if (rc == openni::STATUS_OK)
    {
        rc = m_depthStream.start();
        if (rc != openni::STATUS_OK)
        {
            printf("SimpleViewer: Couldn't start depth stream:\n%s\n", openni::OpenNI::getExtendedError());
            m_depthStream.destroy();
        }
    }
    else
    {
        printf("SimpleViewer: Couldn't find depth stream:\n%s\n", openni::OpenNI::getExtendedError());
    }

    rc = m_colorStream.create(device, openni::SENSOR_COLOR);
    if (rc == openni::STATUS_OK)
    {
        rc = m_colorStream.start();
        if (rc != openni::STATUS_OK)
        {
            printf("SimpleViewer: Couldn't start color stream:\n%s\n", openni::OpenNI::getExtendedError());
            m_colorStream.destroy();
        }
    }
    else
    {
        printf("SimpleViewer: Couldn't find color stream:\n%s\n", openni::OpenNI::getExtendedError());
    }

    if (!m_depthStream.isValid() || !m_colorStream.isValid())
    {
        printf("SimpleViewer: No valid streams. Exiting\n");
        openni::OpenNI::shutdown();
        return openni::STATUS_ERROR;
    }

    return 0;
}

int ImageSource::streamInit()
{
	openni::VideoMode depthVideoMode;
    openni::VideoMode colorVideoMode;

    if (m_depthStream.isValid() && m_colorStream.isValid())
    {
        depthVideoMode = m_depthStream.getVideoMode();
        colorVideoMode = m_colorStream.getVideoMode();

        int depthWidth = depthVideoMode.getResolutionX();
        int depthHeight = depthVideoMode.getResolutionY();
        int colorWidth = colorVideoMode.getResolutionX();
        int colorHeight = colorVideoMode.getResolutionY();

        if (depthWidth == colorWidth && depthHeight == colorHeight) {
            m_width = depthWidth;
            m_height = depthHeight;
        } else {
            printf("Error - expect color and depth to be in same resolution: D: %dx%d, C: %dx%d\n",
                depthWidth, depthHeight,
                colorWidth, colorHeight);
            return openni::STATUS_ERROR;
        }
    } else if (m_depthStream.isValid()) {

        depthVideoMode = m_depthStream.getVideoMode();

        m_width = depthVideoMode.getResolutionX();
        m_height = depthVideoMode.getResolutionY();
    } else if (m_colorStream.isValid()) {
		colorVideoMode = m_colorStream.getVideoMode();

		m_width = colorVideoMode.getResolutionX();
		m_height = colorVideoMode.getResolutionY();
    } else {
        printf("Error - expects at least one of the streams to be valid...\n");
        return openni::STATUS_ERROR;
    }

    m_streams[0] = &m_depthStream;
    m_streams[1] = &m_colorStream;

    return 0;
}

void ImageSource::update() 
{
	int changedIndex;

    openni::Status rc = openni::OpenNI::waitForAnyStream(m_streams, 2, &changedIndex);
    if (rc != openni::STATUS_OK) {
        printf("Wait failed\n");
        return;
    }


    using namespace std;
    switch (changedIndex) {
    case 0:
    {
    	depth_mutex.lock();
        m_depthStream.readFrame(&m_depthFrame);
       	m_sequence = max<int>(m_sequence, m_depthFrame.getFrameIndex());//bug(overflow in ~700 days)
        depth_mutex.unlock();
    }
    	break;
    case 1:
    {
    	color_mutex.lock();
        m_colorStream.readFrame(&m_colorFrame);
        m_sequence = max<int>(m_sequence, m_colorFrame.getFrameIndex());//bug(overflow in ~700 days)
        color_mutex.unlock();
    }
    	break;
    default:
        printf("Error in wait\n");
    }
}


int ImageSource::init(void)
{
	if (m_initialized) {
		return 0;
	}

	m_depthMat.create(m_height, m_width, CV_16UC1);
	m_colorMat.create(m_height, m_width, CV_8UC3);

	if (openni::STATUS_ERROR == deviceInit()) {
		m_initialized = false;
		return -1;
	}
	if (openni::STATUS_ERROR == streamInit()) {
		m_initialized = false;
		return -1;
	}
	m_initialized = true;
	start();

	return 0;
}

bool ImageSource::isInitialized()
{
	return m_initialized;
}

cv::Mat ImageSource::getDepthMat()
{
	depth_mutex.lock();
	if (m_depthFrame.isValid()) {
		memcpy(m_depthMat.data, m_depthFrame.getData(), m_depthFrame.getDataSize());
	}
	depth_mutex.unlock();
	return m_depthMat;
}



cv::Mat ImageSource::getColorMat()
{
	color_mutex.lock();
	if (m_colorFrame.isValid()) {
		memcpy(m_colorMat.data, m_colorFrame.getData(), m_colorFrame.getDataSize());
	}
	color_mutex.unlock();
	return m_colorMat;
}

void ImageSource::run()
{
	while(1) {
		update();
		yieldCurrentThread();
		msleep(1);
	}
}
