#pragma once
#include<QtCore>

class CColorSegmentation {
public:
	CColorSegmentation()
:	m_CrCbCountAll()
,	m_CrCbCountSkin()
	{

	}

	bool buildDatabase(const char * path) {
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
         return false;

		 QTextStream in(&file);
		 if (in.atEnd()) {
			 return false;
		 }
		 do {
			 QString line = in.readLine();
			 if (line.count('\t', Qt::CaseInsensitive) != 3) {
				 return false;
			 }

			bool ok = true;

			int red = line.section('\t',0,0).toInt(&ok);
			int green = line.section('\t',1,1).toInt(&ok);
			int blue = line.section('\t',2,2).toInt(&ok);
			bool skin = line.section('\t',3,3).toInt(&ok)==1;



			QVector<int> YCrCb(YCrCbFromRGB(red, green, blue));
			if (skin) {
				m_CrCbCountSkin[YCrCb[1]][YCrCb[2]]++;
			}

			m_CrCbCountAll[YCrCb[1]][YCrCb[2]]++;
//			if (m_CrCbCountAll[YCrCb[1]][YCrCb[2]] != m_CrCbCountSkin[YCrCb[1]][YCrCb[2]]) {
//				if (m_CrCbCountSkin[YCrCb[1]][YCrCb[2]])
//					qDebug("[%d %d]: %d %d", YCrCb[1], YCrCb[2], m_CrCbCountSkin[YCrCb[1]][YCrCb[2]], m_CrCbCountAll[YCrCb[1]][YCrCb[2]]);
//			}

		 } while (!in.atEnd());
		 return true;
	}

	static QVector<int> YCrCbFromRGB(uint8_t red, uint8_t green, uint8_t blue)
	{
		QVector<int> YCrCb(3);

		YCrCb[0] = 0.257*red + 0.504*green + 0.098*blue + 16;
		YCrCb[1] = 0.439*red - 0.368*green - 0.071*blue + 128;
		YCrCb[2] = -0.148*red - 0.291*green + 0.439*blue + 128;

 		return YCrCb;
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
		return getProbability(uv[0], uv[1]);
	}

	inline double getProbability(const cv::Point3_<uint8_t> bgr)
	{
		return getProbability(bgr.z, bgr.y, bgr.x);
	}

	void scanNewImage() {
		// TODO:
		// Format: 	* RGB bitmap 8+8+8
		// 			* Metadata consisting of vertices of lines in image (convex regions)
		//				Region1: (x1,y1), (x2,y2), ..., (xn,yn)\n
		//				Region2: ...
	}

	uint32_t m_CrCbCountAll[256][256];
	uint32_t m_CrCbCountSkin[256][256];
};
