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


#pragma once
#include <opencv2/opencv.hpp>

#include <QtCore/qmutex.h>
#include <libfreenect/libfreenect.hpp>
#include "ImageSource.h"

namespace iez_private {
	class ImageSourceFreenectDevice_private;
}


namespace iez {
class ImageSourceFreenect:public ImageSource
{
public:
	ImageSourceFreenect(const int index = 0);
	void streamInit(freenect_resolution resolution);
	cv::Mat getColorMat() const;
	cv::Mat getDepthMat() const;

	~ImageSourceFreenect() { freenect.deleteDevice(0); }
private:
	Freenect::Freenect freenect;
	iez_private::ImageSourceFreenectDevice_private *device;


};

}
