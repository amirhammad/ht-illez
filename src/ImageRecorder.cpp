#include "ImageRecorder.h"
using namespace openni;
namespace iez {
ImageRecorder::ImageRecorder(iez::ImageSourceOpenNI *src, const char *path)
{
	m_recorder = new Recorder();
	m_recorder->create(path);
	m_recorder->attach(src->getColorStream(), false);
	m_recorder->attach(src->getDepthStream(), false);
	m_recorder->start();
}

ImageRecorder::~ImageRecorder()
{
	m_recorder->destroy();
	delete m_recorder;
}

}
