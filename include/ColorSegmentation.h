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
#include<QtCore>
#include <opencv2/opencv.hpp>
#include <QtGui/qimage.h>
#include <QtGui/qslider.h>
#include <list>
#include "WindowManager.h"

namespace iez {
class ImageStatistics {
public:
	ImageStatistics();
	ImageStatistics(const cv::Mat &image, const bool skin = false);
	void processPixel(const cv::Point3_<uint8_t> &tcsPoint, bool skin = false);
	double getProbability(uint8_t u, uint8_t v) const;
	cv::Mat getProbabilityMap() const;
	cv::Mat getProbabilityMapComplementary(const cv::Mat &image, const std::list<ImageStatistics>& statsList, const float mul = 1);
	cv::Mat getProbabilityMap(const cv::Mat &bgr) const;

	cv::Mat getCountAllMapNormalized() const;
	void clear();
	long getSampleCount() const { return m_pixelCounter; }
private:

	cv::Mat getCountSkinMapNormalized();



	void finalize();



private:
	long m_tcsNonSkinCounter[256][256];
	long m_tcsSkinCounter[256][256];
	long m_pixelCounter;
	long m_maxCountSkin;
	long m_maxCountNonSkin;
	long m_skinCounter;
};

class ColorSegmentation : QObject {
	Q_OBJECT
public:
	ColorSegmentation();
//	bool buildDatabaseFromRGBS(const char * path); // deprecated
	~ColorSegmentation();

	static bool buildDatabaseFromFiles(const QString &path);
	static std::list<QPolygon> polygonsFromFile(const QString &imagePath);
	static void scanNewImage(const cv::Mat &image, const std::list<QPolygon> &polygonList);
	static const std::vector<int> separateSkinNonskinColorInRow(int row, const std::list<QPolygon> &polygonList);

	double getTMax() const
	{
		return m_TMax;
	}

	double getTMin() const
	{
		return m_TMin;
	}

	static ImageStatistics m_statsFile;
	void saveStats(QString path);
	/**
	 * Separate Skin/Non-skin color in row
	 * return list of area changes(columns)
	 */



private:
	QWidget *m_master;
	QSlider *m_sliderTMin;
	QSlider *m_sliderTMax;
	double m_TMax;
	double m_TMin;
private slots:
	void on_TMaxChanged(int value);
	void on_TMinChanged(int value);

};

}
