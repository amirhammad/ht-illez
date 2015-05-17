/*
 * This file is part of the project HandTrackerApp - ht-illez
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
