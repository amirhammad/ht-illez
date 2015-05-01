#include "ImageSourceArtificial.h"

cv::Mat iez::ImageSourceArtificial::getColorMat() const
{
        QMutexLocker locker(&m_mutex);
        cv::Mat ret;
        m_color.copyTo(ret);
        return ret;
}

cv::Mat iez::ImageSourceArtificial::getDepthMat() const
{
        QMutexLocker locker(&m_mutex);
        return m_depth;
}

iez::ImageSourceArtificial::ImageSourceArtificial()
:	ImageSource()
{
        QMutexLocker locker(&m_mutex);
        m_color.create(480, 640, CV_8UC3);
}

iez::ImageSourceArtificial::~ImageSourceArtificial()
{
}

void iez::ImageSourceArtificial::setColorMat(const cv::Mat& src)
{
        QMutexLocker locker(&m_mutex);
        cvtColor(src, m_color, cv::COLOR_RGB2BGR);
        m_sequence++;
}

void iez::ImageSourceArtificial::setDepthMat(const cv::Mat &src)
{
        QMutexLocker locker(&m_mutex);
		m_depth = src;
}

iez::ImageSourceArtificial *iez::ImageSourceArtificial::globalInstance()
{
	static ImageSourceArtificial artif;
	return &artif;
}

void iez::ImageSourceArtificial::pause(bool p)
{
        Q_UNUSED(p);
}
