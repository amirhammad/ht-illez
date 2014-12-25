#pragma once
#include<QtCore>
#include <opencv2/opencv.hpp>
#include <QtGui/qimage.h>
#include "WindowManager.h"

namespace iez {

class ColorSegmentation {
public:
	ColorSegmentation()
:	m_CrCbCountAll()
,	m_CrCbCountSkin()
, 	maxSkin(0)
,	maxAll(0)
	{

	}

	bool buildDatabaseFromRGBS(const char * path);

	bool buildDatabaseFromSingleImage(const cv::Mat &img);

	std::list<QPolygon> polygonsFromFile(const QString &imagePath);

	bool buildDatabaseFromFiles(const QString &path);

	static QVector<int> YCrCbFromRGB(uint8_t red, uint8_t green, uint8_t blue)
	{
		QVector<int> YCrCb(3);

		YCrCb[0] = 0.257*red + 0.504*green + 0.098*blue + 16;
		YCrCb[1] = 0.439*red - 0.368*green - 0.071*blue + 128;
		YCrCb[2] = -0.148*red - 0.291*green + 0.439*blue + 128;

 		return YCrCb;
	};

	static QVector<int> YCrCbFromRGB(const cv::Point3_<uint8_t> bgr)
	{
 		return YCrCbFromRGB(bgr.z, bgr.y, bgr.x);
	};

	inline double getProbability(uint8_t u, uint8_t v)
	{
		if (m_CrCbCountAll[u][v]) {
			return m_CrCbCountSkin[u][v]/m_CrCbCountAll[u][v];
		} else {
			return 0;
		}
	}
	inline double getProbability(uint8_t red, uint8_t green, uint8_t blue)
	{
		QVector<int> uv = YCrCbFromRGB(red, green, blue);
		return getProbability(uv[1], uv[2]);
	}

	inline double getProbability(const cv::Point3_<uint8_t> bgr)
	{
		return getProbability(bgr.z, bgr.y, bgr.x);
	}

	/**
	 * Separate Skin/Non-skin color in row
	 * return list of areas
	 */
	const std::vector<int> separateSkinNonskinColorInRow(int row, const std::list<QPolygon> polygonList);

	void loadNewImage(const char *imagePath);
	void scanNewImage(const cv::Mat &image, const std::list<QPolygon> &polygonList);

	uint32_t maxAll;
	uint32_t maxSkin;
	uint32_t m_CrCbCountAll[256][256];
	uint32_t m_CrCbCountSkin[256][256];

	WindowManager windowManager;
};

}
