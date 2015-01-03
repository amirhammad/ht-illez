
#include "ColorSegmentation.h"
#include "WindowManager.h"
#include "Processing.h"
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



cv::Mat ImageStatistics::getProbabilityMap(const cv::Mat &bgr) const
{
	cv::Mat yuv;
	cvtColor(bgr, yuv, cv::COLOR_BGR2YUV);
	cv::Mat map;
	map.create(bgr.rows, bgr.cols, CV_32F);
	for (int i = 0; i < bgr.rows; i++) {
		for (int j = 0; j < bgr.cols; j++) {
			const cv::Point3_<uint8_t> &point = yuv.at<cv::Point3_<uint8_t>>(i,j);
			map.at<float>(i,j) = getProbability(point.y, point.z);
		}
	}
	return map;
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
		if (!imagePath.endsWith(".bmp", Qt::CaseInsensitive)) {
			imagePath.append(".bmp");
		}
		qDebug("%s", imagePath.toStdString().data());
		QImage image;
		if (!image.load(QString(DATABASE_PREFIX).append(imagePath))) {

			qDebug("Error: cannot load image'%s'", imagePath.toStdString().data());
		}
		image = image.convertToFormat(QImage::Format_RGB888);
		const std::list<QPolygon> polygons = polygonsFromFile(imagePath);
		const cv::Mat &bgr = WindowManager::QImage2Mat(image);

		scanNewImage(bgr, polygons);

	 } while (!in.atEnd());

	 //saveStats(path);
	 WindowManager::getInstance().imShow("FileDB statistics", m_statsFile.getProbabilityMap());

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
	static int id = 0;
	cv::Mat maskedImage;
	cv::Mat yuv;
	cvtColor(image, yuv, cv::COLOR_BGR2YUV);
	image.copyTo(maskedImage);
	for (int i = 0; i< image.rows; i++) {
		const std::vector<int> &areas = separateSkinNonskinColorInRow(i, polygonList);

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

			m_statsFile.processPixel(yuv.at<const cv::Point3_<uint8_t> >(i,j), skin);
			// Categorize .. .TODO;
			if (skin) {
				maskedImage.at<cv::Point3_<uint8_t> >(i,j) = cv::Point3_<uint8_t>(0, 0, 0);
			}


		}


	}
//	WindowManager::getInstance().imShow(QString("segmented: %1").arg(id++), maskedImage);

//	cv::Mat bigger(768, 768, CV_8UC1);
//	cv::resize(m_statsFile.getCountAllMapNormalized(), bigger, bigger.size(), 0, 0, cv::INTER_NEAREST);
//	WindowManager::getInstance().imShow("statsFile", bigger);
//	cv::resize(m_statsFile.getCountSkinMapNormalized(), bigger, bigger.size(), 0, 0, cv::INTER_NEAREST);
//	WindowManager::getInstance().imShow("statsFileSkin", bigger);
}


/**
 * Separate Skin/Non-skin color in row
 * return list of areas
 */
const std::vector<int> ColorSegmentation::separateSkinNonskinColorInRow(int row, const std::list<QPolygon> polygonList)
{
	std::vector<int> edgePointsVector(30, 0);

	int index = 0;
	foreach (QPolygon polygon, polygonList) {

		for (int i = 0; i < polygon.size()-1; ++i) {
			const QPoint &curr = polygon.at(i);
			const QPoint &next = polygon.at(i+1);

			if (curr.y() == next.y()) {
				continue;
//				edgePointsVector[index++] = curr.x();
//				edgePointsVector[index++] = next.x();
			} else if ((curr.y() <= row && row < next.y())
					|| (curr.y() > row && row >= next.y())) {
				float k = static_cast<float>(next.x()-curr.x())/(next.y() - curr.y());
				float q = curr.x()-k*curr.y();
				float x = k*row+q;
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



/**
 * \arg &tColorSpace - pixel in whatever colorspace(must be max. 3x8bit)
 *
 */
void ImageStatistics::processPixel(const cv::Point3_<uint8_t> &tcsPoint, bool skin)
{
	long &tcsSkinCounter = m_tcsSkinCounter[tcsPoint.y][tcsPoint.z];
	long &tcsNonSkinCounter = m_tcsNonSkinCounter[tcsPoint.y][tcsPoint.z];

	if (skin) {
		tcsSkinCounter++;
		m_skinCounter++;
	} else {
		tcsNonSkinCounter++;
	}

	if (!(tcsPoint.y == 128 && tcsPoint.z == 128)) {
		// Find max
		m_maxCountSkin = std::max<long>(m_maxCountSkin, tcsSkinCounter);
		m_maxCountNonSkin = std::max<long>(m_maxCountNonSkin, tcsNonSkinCounter);
	}

	m_pixelCounter++;
}

cv::Mat ImageStatistics::getCountAllMapNormalized() const
{
	cv::Mat map;
	map.create(256, 256, CV_8UC1);
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			const uint32_t &counterAll = m_tcsNonSkinCounter[i][j];
			double val = std::min<double>(1.0, static_cast<double>(counterAll)/m_maxCountNonSkin);
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
			const uint32_t &counter = m_tcsSkinCounter[i][j];
			double val = static_cast<double>(counter)/m_maxCountSkin;
			map.at<uint8_t>(i,j) = val*255;
		}
	}
	return map;
}


void ImageStatistics::clear()
{
	m_pixelCounter = m_maxCountSkin = m_maxCountNonSkin = m_skinCounter = 0;
	memset(m_tcsNonSkinCounter, 0, 256*256*sizeof(m_tcsSkinCounter[0][0]));
	memset(m_tcsSkinCounter, 0, 256*256*sizeof(m_tcsSkinCounter[0][0]));
}

ImageStatistics::ImageStatistics()
:	m_pixelCounter(0)
,	m_maxCountSkin(0)
,	m_maxCountNonSkin(0)
,	m_skinCounter(0)
{
	memset(m_tcsNonSkinCounter, 0, sizeof(m_tcsNonSkinCounter));
	memset(m_tcsSkinCounter, 0, sizeof(m_tcsSkinCounter));
}

ImageStatistics::ImageStatistics(const cv::Mat &image, const bool skin)
:	ImageStatistics()
{
	cv::Mat yuv;
	cv::cvtColor(image, yuv, cv::COLOR_BGR2YUV);
	for (int i = 0; i < image.rows; i++) {
		for (int j = 0; j < image.cols; j++) {
			const cv::Point3_<uint8_t> &pt = yuv.at<cv::Point3_<uint8_t> >(i, j);
			const cv::Point3_<uint8_t> &ptBgr = image.at<cv::Point3_<uint8_t> >(i, j);
			if (ptBgr.x + ptBgr.y + ptBgr.z == 0) {
				continue;
			} else {
				processPixel(pt, skin);
			}
		}
	}
}

cv::Mat ImageStatistics::getProbabilityMap() const
{
	cv::Mat map;
	map.create(256, 256, CV_8UC1);
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			map.at<uint8_t>(i,j) = getProbability(i,j)*255;
		}
	}
	return map;
}

/**
 *
 * \param ratio ratio between original and complementary images
 * 			original:complementary
 */
cv::Mat ImageStatistics::getProbabilityMapComplementary(const cv::Mat &image
,	const std::list<ImageStatistics>& statsList
,	const float ratio)
{

//	const float mult = statsList.size()/ratio;
	cv::Mat ret;
	ImageStatistics statsSum(*this);
	foreach (ImageStatistics stats, statsList) {
//		statsSum.m_pixelCounter += stats.m_pixelCounter/ratio;
//		statsSum.m_skinCounter += stats.m_skinCounter/ratio;
		for (int i = 0; i < 256; i++) {
			for (int j = 0; j < 256; j++) {
//				const float mul = ((image.cols*image.rows/stats.m_pixelCounter)/ratio);
				const long inc = stats.m_tcsSkinCounter[i][j]/ratio;
				statsSum.m_tcsSkinCounter[i][j] += inc;
				statsSum.m_skinCounter += inc;
				statsSum.m_pixelCounter += inc;
			}
		}
	}
	return statsSum.getProbabilityMap(image);
}

void ImageStatistics::finalize()
{

}

double ImageStatistics::getProbability(uint8_t u, uint8_t v) const
{
	if (!m_pixelCounter) return 0;
//	double Pc = double(m_tcsSkinCounter[u][v] + m_tcsNonSkinCounter[u][v])/m_pixelCounter;
	double Pc = double(m_tcsSkinCounter[u][v] + m_tcsNonSkinCounter[u][v])/double(m_pixelCounter);
	Q_ASSERT(Pc <= 1);
//	double Ps = double(m_skinCounter)/m_pixelCounter;
	double Ps = double(m_tcsSkinCounter[u][v])/double(m_pixelCounter);
	Q_ASSERT(Ps <= 1);
	if (m_tcsSkinCounter[u][v]+m_tcsNonSkinCounter[u][v] == 0) return 0.5;
//	double Pcs = double(m_tcsSkinCounter[u][v])/(m_tcsSkinCounter[u][v]+m_tcsNonSkinCounter[u][v]);
	double Pcs = double(m_tcsSkinCounter[u][v])/double(m_tcsSkinCounter[u][v]+m_tcsNonSkinCounter[u][v]);
///
///
///	double PcsDivPc = double(m_tcsSkinCounter[u][v])*m_pixelCounter;
///	return Pcs*Ps/Pc;
///	return Ps*pcsDivPc;

	/// EXPERIMENT
	double psDIVpc = double(m_skinCounter)/double(m_tcsSkinCounter[u][v] + m_tcsNonSkinCounter[u][v]);
	/// pcs*ps/pc = double(m_tcsSkinCounter[u][v])/(m_tcsSkinCounter[u][v]+m_tcsNonSkinCounter[u][v])
	double Psc = Pcs*Ps/Pc;
/// ~	double Psc = double(m_skinCounter)*double(m_tcsSkinCounter[u][v]);
//	qDebug("Psc: %f %f %f %f", Psc, Pcs, Ps, Pc);
	Q_ASSERT(Psc<=1);
	Q_ASSERT(Psc>=0);

	return Psc;
}

#define SLIDER_TICKS 1000

ColorSegmentation::ColorSegmentation()
: 	m_TMax(0.5)
,	m_TMin(0.15)
,	QObject()
{
	m_master = new QWidget();
	m_master->setWindowTitle("Color segmenter");

	QVBoxLayout *mainLayout = new QVBoxLayout(m_master);

	m_sliderTMin = new QSlider(m_master);
	m_sliderTMax = new QSlider(m_master);

	auto basicSetup = [] (QSlider *slider) {
		slider->setMinimum(0);
		slider->setMaximum(SLIDER_TICKS);
		slider->setOrientation(Qt::Horizontal);
		slider->setTickInterval(SLIDER_TICKS/10);
		slider->setTickPosition(QSlider::TicksBelow);
	};

	basicSetup(m_sliderTMin);
	basicSetup(m_sliderTMax);


	m_sliderTMin->setValue(m_TMin*SLIDER_TICKS);
	m_sliderTMax->setValue(m_TMax*SLIDER_TICKS);
	mainLayout->addWidget(m_sliderTMin);
	mainLayout->addWidget(m_sliderTMax);
	connect(m_sliderTMin, SIGNAL(valueChanged(int)), this, SLOT(on_TMinChanged(int)));
	connect(m_sliderTMax, SIGNAL(valueChanged(int)), this, SLOT(on_TMaxChanged(int)));
	m_master->setLayout(mainLayout);

	m_master->show();
}

void ColorSegmentation::on_TMaxChanged(int value)
{
	if (value < m_TMin*SLIDER_TICKS)	{
		m_sliderTMin->setValue(value);
	}
	m_TMax = value/float(SLIDER_TICKS);
}

ColorSegmentation::~ColorSegmentation()
{
	delete m_master;
}

void ColorSegmentation::on_TMinChanged(int value)
{
	if (value > m_TMax*SLIDER_TICKS) {
		m_sliderTMax->setValue(value);
	}

	m_TMin = value/float(SLIDER_TICKS);
}





}


