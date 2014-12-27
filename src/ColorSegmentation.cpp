
#include "ColorSegmentation.h"
#include "WindowManager.h"

#include <QtGui>
#include <QtCore>
namespace iez {
ImageStatistics ColorSegmentation::m_statsFile;

#define DATABASE_PREFIX "../database/"
std::list<QPolygon> ColorSegmentation::polygonsFromFile(const QString &imagePath)
{
	std::list<QPolygon> polygonList;
	QFile imageMeta(QString(DATABASE_PREFIX).append(QString(imagePath)).append("_meta"));
	if (!imageMeta.open(QIODevice::ReadOnly | QIODevice::Text))
		return polygonList;
	QTextStream imageMetaStream(&imageMeta);

	do {
		QString header = imageMetaStream.readLine(64);
		QString type = header.section(' ',0,0);

		if (type.compare("polygons") == 0) {
			int polygonListCount = header.section(' ', 1,1).toInt();
			for (int i = 0; i < polygonListCount; ++i) {
				QPolygon polygon;
				QString polyogonHeader = imageMetaStream.readLine(64);
				int numberOfPoints = polyogonHeader.section(' ', 0, 0).toInt();
				for (int j = 0; j < numberOfPoints; ++j) {
					QString polyogonLine = imageMetaStream.readLine(64);
					int y = polyogonLine.section(' ', 0, 0).toInt();
					int x = polyogonLine.section(' ', 1, 1).toInt();
					polygon.push_back(QPoint(x,y));
				}
				polygonList.push_back(polygon);
			}
		}
	} while(!imageMetaStream.atEnd());
	return polygonList;
}


bool ColorSegmentation::buildDatabaseFromRGBS(const char * path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	 return false;

	 QTextStream in(&file);
	 if (in.atEnd()) {
		 return false;
	 }
	 ImageStatistics stats;
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


		stats.processPixel(cv::Point3_<uint8_t>(blue, green, red), skin);
//		QVector<int> YCrCb(YCrCbFromRGB(red, green, blue));
//		if (skin) {
//			m_CrCbCountSkin[YCrCb[1]][YCrCb[2]]++;
//
//			if (maxSkin < m_CrCbCountSkin[YCrCb[1]][YCrCb[2]]) {
//				maxSkin = m_CrCbCountSkin[YCrCb[1]][YCrCb[2]];
//			}
//		}
//
//		m_CrCbCountAll[YCrCb[1]][YCrCb[2]]++;
//		if (maxAll < m_CrCbCountAll[YCrCb[1]][YCrCb[2]]) {
//			maxAll = m_CrCbCountAll[YCrCb[1]][YCrCb[2]];
//		}

	 } while (!in.atEnd());
	 return true;
}


bool ColorSegmentation::buildDatabaseFromSingleImage(const cv::Mat &img)
{
	cv::Mat filtered;
	cv::cvtColor(img, filtered, cv::COLOR_BGR2GRAY);
	filtered.create(img.rows, img.cols, CV_8UC1);
	ImageStatistics stats;
	for (int i = 0; i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			const cv::Point3_<uint8_t> &point = img.at<cv::Point3_<uint8_t> >(i,j);
			stats.processPixel(point, false);
//			if (m_statsFile.getProbability(point) > 0.000000000000005) {
//				filtered.at<uint8_t> (i,j) = 0;
//			} else {
//
//			}
		}
	}

	WindowManager::getInstance().imShow("statistics", stats.getCountAllMapNormalized());
//	WindowManager().imShow("statistics", stats.getCountAllMapNormalized());
//	WindowManager::getInstance().imShow("filtered", filtered);
	return true;
}

bool ColorSegmentation::buildDatabaseFromFiles(const QString &path)
{

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;


	 QTextStream in(&file);
	 if (in.atEnd()) {
		 return false;
	 }

	 do {
		QString imagePath = in.readLine();
		if (imagePath.startsWith('#')) {
			continue;
		}
		qDebug("%s", imagePath.toStdString().data());
		QImage image;
		if (!image.load(QString(DATABASE_PREFIX).append(imagePath))) {

			qDebug("Error: cannot load image'%s'", imagePath.toStdString().data());
		}
		image = image.convertToFormat(QImage::Format_RGB888);
		const std::list<QPolygon> polygons = polygonsFromFile(imagePath);

		scanNewImage(WindowManager::QImage2Mat(image), polygons);

	 } while (!in.atEnd());

	 //saveStats(path);

	 return true;
}

void ColorSegmentation::saveStats(QString path) {
	QFile statsFile(QString(path).append("_output.csv"));
	if (!statsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}
	QTextStream statsFileStream(&statsFile);

	 for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
//			uint32_t &counterAll = m_statsFile.m_CrCbCountAll[i][j];
//			uint32_t &counterSkin = m_statsFile.m_CrCbCountSkin[i][j];
//			statsFileStream<<counterAll<<',';
		}
		statsFileStream<<endl;
	}
}


void ColorSegmentation::scanNewImage(const cv::Mat &image, const std::list<QPolygon> &polygonList)
{
	cv::Mat maskedImage;
	image.copyTo(maskedImage);
	for (int i = 0; i< image.rows; i++) {
		const std::vector<int> &areas = separateSkinNonskinColorInRow(i, polygonList);
//		qDebug("areas: %d:", areas.size());
//		foreach (int p, areas) {
//			qDebug("%d", p);
//		}
		int index = 0;
		bool skin = false;
		bool end = false;	//indicates last area
//		qDebug("area[%d]", areas);
		if (areas[0] < 0) {
			end = true;
		}

		for (int j = 0; j < image.cols; j++) {
			if (!end) {
				if (j > areas[index] && areas[index] >= 0) {
//					qDebug("area[%d]", areas[index]);

					index++;
					skin = !skin;
					if (areas[index] < 0) {
						end = true;
					}
				}
			}

//			const QVector<int> &YCrCb = YCrCbFromRGB(image.at<const cv::Point3_<uint8_t> >(i,j));
			m_statsFile.processPixel(image.at<const cv::Point3_<uint8_t> >(i,j), skin);
			// Categorize .. .TODO;
			if (skin) {
				maskedImage.at<cv::Point3_<uint8_t> >(i,j) = cv::Point3_<uint8_t>(0, 0, 0);
			}


		}


	}
//	WindowManager::getInstance().imShow("x", maskedImage);

	cv::Mat bigger(768, 768, CV_8UC1);
	cv::resize(m_statsFile.getCountAllMapNormalized(), bigger, bigger.size(), 0, 0, cv::INTER_NEAREST);
	WindowManager::getInstance().imShow("statsFile", bigger);
	cv::resize(m_statsFile.getCountSkinMapNormalized(), bigger, bigger.size(), 0, 0, cv::INTER_NEAREST);
	WindowManager::getInstance().imShow("statsFileSkin", bigger);
}


/**
 * Separate Skin/Non-skin color in row
 * return list of areas
 */
const std::vector<int> ColorSegmentation::separateSkinNonskinColorInRow(int row, const std::list<QPolygon> polygonList)
{
	std::vector<int> edgePointsVector(20, 0);

	int index = 0;
	foreach (QPolygon polygon, polygonList) {

		for (int i = 0; i < polygon.size()-1; ++i) {
			const QPoint &curr = polygon.at(i);
			const QPoint &next = polygon.at(i+1);

			if (curr.y() == next.y()) {
				edgePointsVector[index++] = curr.x();
				if (curr.x() != next.x()) {
					edgePointsVector[index++] = next.x();
				}
			} else if ((curr.y() <= row && row < next.y())
					|| (curr.y() > row && row >= next.y())) {
				float k = static_cast<float>(next.x()-curr.x())/(next.y() - curr.y());
				int q = curr.x()-k*curr.y();
				int x = k*row+q;
				edgePointsVector[index++] = x;
			}
		}

		//and last

		const QPoint &first = polygon.at(0);
		const QPoint &last = polygon.at(polygon.size()-1);
		if ((first.y() <= row && row < last.y())
		|| (first.y() > row && row >= last.y())) {
			float k = static_cast<float>(last.x()-first.x())/(last.y() - first.y());
			int q = first.x()-k*first.y();
			int x = k*row+q;
			edgePointsVector[index++] = x;
		}
	}

	if (index&1) index--;
	std::sort(edgePointsVector.begin(), edgePointsVector.begin()+index);

	edgePointsVector[index] = -1;
	return edgePointsVector;
}



void ImageStatistics::processPixel(const cv::Point3_<uint8_t> bgr, bool skin)
{
	QVector<int> YCrCb(YCrCbFromRGB(bgr));
	uint32_t &counterSkin = m_CrCbCountSkin[YCrCb[1]][YCrCb[2]];
	uint32_t &counterAll = m_CrCbCountAll[YCrCb[1]][YCrCb[2]];

	if (skin) {
		counterSkin++;
	}

	counterAll++;

	// Find max
	m_maxCountSkin = std::max<uint32_t>(m_maxCountSkin, counterSkin);
	m_maxCountAll = std::max<uint32_t>(m_maxCountAll, counterAll);

	m_pixelCounter++;
}

cv::Mat ImageStatistics::getCountAllMapNormalized()
{
	cv::Mat map;
	map.create(256, 256, CV_8UC1);
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			const uint32_t &counterAll = m_CrCbCountAll[i][j];
			double val = static_cast<double>(counterAll)/m_maxCountAll;
			map.at<uint8_t>(i,j) = val*255;
		}
	}
	return map;
}

cv::Mat ImageStatistics::getCountSkinMapNormalized()
{
	cv::Mat map;
	map.create(256, 256, CV_8UC1);
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			const uint32_t &counter = m_CrCbCountSkin[i][j];
			double val = static_cast<double>(counter)/m_maxCountSkin;
			map.at<uint8_t>(i,j) = val*255;
		}
	}
	return map;
}


ImageStatistics::ImageStatistics()
:	m_pixelCounter(0)
,	m_maxCountSkin(0)
,	m_maxCountAll(0)
{
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			uint32_t &counterAll = m_CrCbCountAll[i][j];
			counterAll = 0;
			uint32_t &counterSkin = m_CrCbCountSkin[i][j];
			counterSkin = 0;
//			double val = static_cast<double>(counterAll);///m_maxCountAll;
//			map.at<uint8_t>(i,j) = val;
		}
	}
}

void ImageStatistics::finalize()
{

}

}
