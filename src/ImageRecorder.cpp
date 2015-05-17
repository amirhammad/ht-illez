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


#include "ImageRecorder.h"
#include <iostream>
using namespace openni;
namespace iez {
ImageRecorder::ImageRecorder()
{
}

void ImageRecorder::init(iez::ImageSourceOpenNI *src, QString path)
{
	m_recorder.create(path.toStdString().c_str());
	m_recorder.attach(src->getColorStream(), false);
	m_recorder.attach(src->getDepthStream(), false);
	m_recorder.start();
}

ImageRecorder::~ImageRecorder()
{
	m_recorder.destroy();
}

}
