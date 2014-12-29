#pragma once
#include<QtCore>
#include <opencv2/opencv.hpp>
#include <QtGui/qimage.h>
#include "WindowManager.h"

namespace iez {

class ImageStatistics {
public:
	ImageStatistics();
	ImageStatistics(const cv::Mat &img);
	static QVector<int> YCrCbFromRGB(float red, float green, float blue)
	{
		QVector<int> YCrCb(3);

		YCrCb[0] = 0.257f*red + 0.504f*green + 0.098f*blue + 16.f;
		YCrCb[1] = 0.439f*red - 0.368f*green - 0.071f*blue + 128.f;
		YCrCb[2] = -0.148f*red - 0.291f*green + 0.439f*blue + 128.f;

 		return YCrCb;
	}

	static QVector<int> YCrCbFromRGB(const cv::Point3_<uint8_t> &bgr)
	{
 		return YCrCbFromRGB(float(bgr.z), float(bgr.y), float(bgr.x));
	}

	double getProbability(uint8_t u, uint8_t v)
	{
		if (m_maxCountAll) {
//			return m_CrCbCountSkin[u][v]/m_CrCbCountAll[u][v];
			double Pc = m_CrCbCountAll[u][v]/m_maxCountAll;
			double Ps = m_CrCbCountSkin[u][v]/m_maxCountSkin;
			double Pcs = m_CrCbCountSkin[u][v]/m_maxCountAll;
			return Pcs*Ps/Pc;
		} else {
			return 0;
		}
	}

	double getProbability(uint8_t red, uint8_t green, uint8_t blue)
	{
		QVector<int> uv = YCrCbFromRGB(red, green, blue);
		return getProbability(uv[1], uv[2]);
	}

	double getProbability(const cv::Point3_<uint8_t> &bgr)
	{
		return getProbability(bgr.z, bgr.y, bgr.x);
	}

	cv::Mat getCountAllMapNormalized() const;
	cv::Mat getCountSkinMapNormalized();
	void processPixel(const cv::Point3_<uint8_t> &bgr, bool skin = false);
	void finalize();



private:
	long m_CrCbCountAll[256][256];
	long m_CrCbCountSkin[256][256];
	long m_pixelCounter;
	long m_maxCountSkin;
	long m_maxCountAll;
};

class ColorSegmentation {
public:
	bool buildDatabaseFromRGBS(const char * path);



	std::list<QPolygon> polygonsFromFile(const QString &imagePath);

	bool buildDatabaseFromFiles(const QString &path);

	void saveStats(QString path);
	static ImageStatistics m_statsFile;
	/**
	 * Separate Skin/Non-skin color in row
	 * return list of areas
	 */
	const std::vector<int> separateSkinNonskinColorInRow(int row, const std::list<QPolygon> polygonList);

	void loadNewImage(const char *imagePath);
	void scanNewImage(const cv::Mat &image, const std::list<QPolygon> &polygonList);
};

}
