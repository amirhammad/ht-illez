#pragma once
#include<QtCore>
#include <opencv2/opencv.hpp>
#include <QtGui/qimage.h>
#include <QtGui/qslider.h>
#include "WindowManager.h"

namespace iez {
class ImageStatistics {
public:
	ImageStatistics();

	void processPixel(const cv::Point3_<uint8_t> &tcsPoint, bool skin = false);
	double getProbability(uint8_t u, uint8_t v) const;
	cv::Mat getProbabilitiesMap() const;
	cv::Mat getProbabilitiesMapFromImage(const cv::Mat &bgr) const;

private:
	cv::Mat getCountAllMapNormalized() const;
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
	static const std::vector<int> separateSkinNonskinColorInRow(int row, const std::list<QPolygon> polygonList);

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
