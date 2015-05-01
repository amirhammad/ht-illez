#include "ImageDescriptor.h"
#include "ImageSourceFreenect.h"
#include "ColorSegmentation.h"
#include <opencv2/opencv.hpp>
using namespace cv;

namespace iez {

//ImageDescriptorImage::ImageDescriptorImage()
//,	QLabel("x")
//{
//	setWindowTitle("Image Description");
//	setText("a");
//	setMouseTracking(true);
//	setFocusPolicy(Qt::ClickFocus);
//	setCursor(Qt::CrossCursor);
//}

ImageDescriptorImage::ImageDescriptorImage(ImageSource *kinect)
:	m_kinect(kinect)
,	QLabel("x")
{
	if (!kinect) {
		m_cap.open(0);
	}
	setWindowTitle("Image Description");
	setText("a");
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	setCursor(Qt::CrossCursor);
}

void ImageDescriptorImage::mousePressEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton) {
		pressed = true;
		m_pointList.push_back(QPoint(ev->x(), ev->y()));
		repaint();
	} else if (ev->button() == Qt::RightButton) {
//		m_pointList
	}
}

void ImageDescriptorImage::mouseMoveEvent(QMouseEvent *ev)
{
//	if (ev->button() == Qt::LeftButton) {
//		qDebug("mouse move");
	if (pressed) {
		m_pointList.back().setX(ev->x());
		m_pointList.back().setY(ev->y());
		repaint();
	}
//		m_pointList.push_back(QPoint(ev->x(), ev->y()));
//	}
}

void ImageDescriptorImage::mouseReleaseEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton) {
		pressed = false;
	}
}



void ImageDescriptorImage::paintEvent(QPaintEvent *ev)
{
	const int circleSize = 5;

	QLabel::paintEvent(ev);

	QPainter painter(this);
	if (m_pointList.size()==1) {
		painter.drawEllipse(*m_pointList.begin(), circleSize, circleSize);
//		painter.drawPoint(m_pointList.back());
	}

	foreach (QPolygon p, m_polygonList) {
		QBrush brush;
		brush.setColor(QColor::fromRgb(255,0,0));
		painter.setBrush(brush);
		painter.drawConvexPolygon(p);
	}
	if (m_pointList.size()>1) {
		painter.drawEllipse(*m_pointList.begin(), circleSize, circleSize);
		for (std::list<QPoint>::iterator it = ++m_pointList.begin(); it != m_pointList.end(); it++) {
			std::list<QPoint>::iterator prev = it;
			prev--;
			painter.drawLine(*prev, *it);
			painter.drawEllipse(*it, circleSize, circleSize);
		}
	}
	painter.end();

}

void ImageDescriptorImage::keyPressEvent(QKeyEvent *ev)
{
	int key = ev->key();
	switch (key){
	case Qt::Key_C:
		m_pointList.clear();
		m_polygonList.clear();
		break;
	case Qt::Key_U:
		if (!m_pointList.empty()) {
			m_pointList.pop_back();
		}
		break;

	case Qt::Key_A:
		if (m_pointList.size() >= 3) {
			QPolygon polygon(0);
			foreach(QPoint pt, m_pointList) {
				polygon.push_back(pt);
			}
			m_pointList.clear();
			m_polygonList.push_back(polygon);
//			emit polygonSelected(polygon);
		}
		break;
	case Qt::Key_S:
	{
		if (!m_polygonList.size()) {
			break;
		}
		emit descriptionComplete(m_image, m_polygonList);
	}
		break;

	}

	repaint();

}


ImageDescriptor::ImageDescriptor(ImageSource *kinect)
{
	m_backgroundImage = new ImageDescriptorImage(kinect);

	master = new QWidget();
	master->setWindowTitle("Image Descriptor");

	m_polygonsWidget = new QWidget(master);
	QHBoxLayout *mainLayout = new QHBoxLayout(master);

	connect(m_backgroundImage, SIGNAL(descriptionComplete(const QImage&, const std::list<QPolygon>)), this, SLOT(on_descriptionComplete(const QImage&, const std::list<QPolygon>)));

//	QPushButton *resetButton = new QPushButton("load image");
//	connect(resetButton, SIGNAL(clicked()), m_backgroundImage, SLOT(refresh()));

	QPushButton *pauseButton = new QPushButton("Pause");
	connect(pauseButton, SIGNAL(clicked()), this, SLOT(pause()));



	QVBoxLayout *polygonLayout = new QVBoxLayout(m_polygonsWidget);
	mainLayout->addWidget(m_backgroundImage);
	mainLayout->addWidget(m_polygonsWidget);
//	static_cast<QVBoxLayout*>(m_polygonsWidget->layout())->addWidget(resetButton, false, Qt::AlignTop);
	static_cast<QVBoxLayout*>(m_polygonsWidget->layout())->addWidget(pauseButton, false, Qt::AlignTop);
	master->setLayout(mainLayout);

	//Timer
	connect(&m_timer, SIGNAL(timeout()), m_backgroundImage, SLOT(refresh()));
	m_timer.start(30);

	master->show();
}

ImageDescriptor::~ImageDescriptor()
{
	delete master;
}

void ImageDescriptorImage::refresh()
{
//	qDebug("dt: %ld",(clock()-m_fpsCounter)/1000);
	m_fpsCounter = clock();
	Mat img;
	if (m_kinect) {
		const Mat &rgb = m_kinect->getColorMat();
		cvtColor(rgb, img, COLOR_RGB2BGR);
	} else {
		m_cap>>img;
	}

	m_image = iez::WindowManager::Mat2QImage(img);
	refresh(m_image);
}

void ImageDescriptorImage::refresh(const QImage &image)
{
	m_image = image;
	setFixedSize(image.width(), image.height());
	setPixmap(QPixmap::fromImage(image));
}

void ImageDescriptor::on_descriptionComplete(const QImage& image, const std::list<QPolygon> polygonList)
{
	m_image = image;
	m_polygonList = polygonList;
	QFileDialog *dialog = new QFileDialog(master, QString("Save Image and metadata"), QString("../database"), QString("*.bmp"));
	dialog->show();
	connect(dialog, SIGNAL(fileSelected(const QString &)), this, SLOT(on_descriptionFileSelected(const QString &)));
}

void ImageDescriptor::on_descriptionFileSelected(const QString &file)
{
	QString filePath(file);

	if (!file.endsWith(".bmp", Qt::CaseInsensitive)) {
		filePath.append(".bmp");
	}

	qDebug("%s", filePath.toAscii().constData());

	// save image
	if (!m_image.save(filePath, "BMP", 100)) {
		qDebug("Failed to save image %s", filePath.toStdString().data());
		return;
	}

	// save points
	QFile imageMeta(QString(filePath).append("_meta"));
	if (!imageMeta.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug("Failed to save image metadata %s", QString(filePath).append("_meta").toStdString().data());
		return;
	}
	QTextStream imageMetaStream(&imageMeta);
	imageMetaStream<<"polygons " <<  m_polygonList.size() << endl;
	foreach (QPolygon polygon, m_polygonList) {
		imageMetaStream << polygon.size() << endl;
		foreach (QPoint point, polygon) {
			imageMetaStream << point.y() << " " << point.x() << endl;
		}
	}
}

void ImageDescriptor::pause()
{
	if (m_timer.isActive()) {
		m_timer.stop();
	} else {
		m_timer.start();
	}
}

}// namespace iez


