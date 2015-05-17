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


#pragma once
#include <opencv2/opencv.hpp>
#include <qobject.h>

namespace iez {

class ImageSource : public QObject {
	Q_OBJECT
public:
	ImageSource(QObject *parent = 0)
	:	m_sequence(-1) {

	}

	virtual cv::Mat getColorMat() const = 0;
	virtual cv::Mat getDepthMat() const = 0;

	virtual ~ImageSource(){}

	int getSequence() const {
		return m_sequence;
	}

protected:
	int m_sequence;

public slots:
	/**
	 *
	 */
	virtual void pause(bool p = true) = 0;
signals:
	void frameReceived();
};

}
