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


#include "ImageSource.h"

#include <opencv2/opencv.hpp>
#include <QMutex>
#include <QObject>

namespace iez {
class ImageSourceArtificial;

class ImageSourceArtificial : public ImageSource {
	Q_OBJECT
public:
	ImageSourceArtificial();
	virtual ~ImageSourceArtificial();
	cv::Mat getColorMat() const;
	cv::Mat getDepthMat() const;
	void setColorMat(const cv::Mat &src);
	void setDepthMat(const cv::Mat &src);

	static ImageSourceArtificial *globalInstance();
public slots:
	void pause(bool p = true);
private:
	cv::Mat m_color;
	cv::Mat m_depth;
	mutable QMutex m_mutex;
};

}
