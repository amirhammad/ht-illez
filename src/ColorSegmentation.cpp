
#include "ColorSegmentation.h"
#include "WindowManager.h"

#include <QtGui>
#include <QtCore>
namespace iez {

std::list<QPolygon> ColorSegmentation::polygonsFromFile(const QString &imagePath)
{
	std::list<QPolygon> polygonList;
	QFile imageMeta(QString(imagePath).append("_meta"));
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

			if (maxSkin < m_CrCbCountSkin[YCrCb[1]][YCrCb[2]]) {
				maxSkin = m_CrCbCountSkin[YCrCb[1]][YCrCb[2]];
			}
		}

		m_CrCbCountAll[YCrCb[1]][YCrCb[2]]++;
		if (maxAll < m_CrCbCountAll[YCrCb[1]][YCrCb[2]]) {
			maxAll = m_CrCbCountAll[YCrCb[1]][YCrCb[2]];
		}

	 } while (!in.atEnd());
	 return true;
}


bool ColorSegmentation::buildDatabaseFromSingleImage(const cv::Mat &img)
{
	for (int i = 0; i < img.rows; i++) {
		for (int j = 0; j<img.cols; j++) {
			QVector<int> YCrCb(YCrCbFromRGB(img.at<cv::Point3_<uint8_t> >(i,j)));

			m_CrCbCountAll[YCrCb[1]][YCrCb[2]]++;
			if (maxAll < m_CrCbCountAll[YCrCb[1]][YCrCb[2]]) {
				maxAll = m_CrCbCountAll[YCrCb[1]][YCrCb[2]];
			}
		}
	}
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
		QImage image;
		if (!image.load(imagePath)) {
			qDebug("Error: cannot load image'%s'", imagePath.toStdString().data());
		}
		const std::list<QPolygon> polygons = polygonsFromFile(imagePath);
		foreach (QPolygon pol, polygons) {
			qDebug("polygon");
			foreach(QPoint pt, pol) {
				qDebug("%d %d", pt.x(), pt.y());
			}
		}

		scanNewImage(CWindowManager::QImage2Mat(image), polygons);

//			QVector<int> YCrCb(YCrCbFromRGB(red, green, blue));
//			if (skin) {
//				m_CrCbCountSkin[YCrCb[1]][YCrCb[2]]++;
//
//				if (maxSkin < m_CrCbCountSkin[YCrCb[1]][YCrCb[2]]) {
//					maxSkin = m_CrCbCountSkin[YCrCb[1]][YCrCb[2]];
//				}
//			}
//
//			m_CrCbCountAll[YCrCb[1]][YCrCb[2]]++;
//			if (maxAll < m_CrCbCountAll[YCrCb[1]][YCrCb[2]]) {
//				maxAll = m_CrCbCountAll[YCrCb[1]][YCrCb[2]];
//			}

	 } while (!in.atEnd());
	 return true;
}




void ColorSegmentation::scanNewImage(const cv::Mat &image, const std::list<QPolygon> polygonList)
{
	for (int i = 0; i< image.rows; i++) {
		const std::vector<int> &areas = separateSkinNonskinColorInRow(i, polygonList);
		qDebug("areas: %d:", areas.size());
		foreach (int p, areas) {
			qDebug("%d", p);
		}
		int index = 0;
		bool skin = false;
		bool end = false;	//indicates last area
//		std::list<int>::const_iterator  it(areas.begin());
//		qDebug("area[%d]", areas);
		if (areas[0] < 0) {
			end = true;
		}

		for (int j = 0; j < image.cols; j++) {
			if (!end) {
				if (j > areas[index] && areas[index] >= 0) {
					qDebug("area[%d]", areas[index]);

					index++;
					skin = !skin;
					if (areas[index] < 0) {
						end = true;
					}
				}
			}

			const QVector<int> &YCrCb = YCrCbFromRGB(image.at<const cv::Point3_<uint8_t> >(i,j));

			// Categorize .. .TODO;
			if (skin) {
				qDebug("%d: skin pixel: %d %d", index, i, j);
			}


		}


	};
	// TODO:
	// Format: 	* RGB bitmap 8+8+8
	// 			* Metadata consisting of vertices of lines in image (convex regions)
	//				Region1: (x1,y1), (x2,y2), ..., (xn,yn)\n
	//				Region2: ...
}


/**
 * Separate Skin/Non-skin color in row
 * return list of areas
 */
const std::vector<int> ColorSegmentation::separateSkinNonskinColorInRow(int row, const std::list<QPolygon> polygonList)
{
	std::vector<int> edgePointsVector(10, 0);

	int index = 0;
	foreach (QPolygon polygon, polygonList) {

		for (int i = 0; i < polygon.size()-1; ++i) {
			const QPoint &curr = polygon.at(i);
			const QPoint &next = polygon.at(i+1);

			if (curr.y() == next.y()) continue;

			if ((curr.y() < row && row < next.y())
			|| (curr.y() > row && row > next.y())) {
				float k = static_cast<float>(next.x()-curr.x())/(next.y() - curr.y());
				int q = curr.x()-k*curr.y();
				int x = k*row+q;
				edgePointsVector[index++] = x;
			}
		}

		//and last

		const QPoint &first = polygon.at(0);
		const QPoint &last = polygon.at(polygon.size()-1);
		if ((first.y() < row && row < last.y())
		|| (first.y() > row && row > last.y())) {
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

}
