#include "ImageDescriptor.h"
#include <opencv2/opencv.hpp>
using namespace cv;

namespace iez {

ImageDescriptorImage::ImageDescriptorImage()
:	m_cap(0)
,	QLabel("x")
{
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
	QLabel::paintEvent(ev);
	QPainter painter(this);
	if (m_pointList.size()==1) {
		painter.drawPoint(m_pointList.back());
	}
	if (m_pointList.size()>1) {

		for (std::list<QPoint>::iterator it = ++m_pointList.begin(); it != m_pointList.end(); it++) {
			std::list<QPoint>::iterator prev = it;
			prev--;
			painter.drawLine(*prev, *it);
		}


	}
	painter.end();

}

void ImageDescriptorImage::keyPressEvent(QKeyEvent *ev)
{
	int key = ev->key();
	switch (key){
	case Qt::Key_Escape:
		m_pointList.clear();
		break;
	case Qt::Key_U:
		if (!m_pointList.empty()) {
			m_pointList.pop_back();
		}
		break;

	case Qt::Key_A:
		if (m_pointList.size() >= 3) {
			QPolygon polygon;
			foreach(QPoint pt, m_pointList) {
				polygon.push_back(pt);
				qDebug(".");
			}
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


ImageDescriptor::ImageDescriptor()
{
	master = new QWidget();
	m_polygonsWidget = new QWidget(master);
	QHBoxLayout *mainLayout = new QHBoxLayout(master);
	QPushButton *resetButton = new QPushButton("load image");
	m_backgroundImage = new ImageDescriptorImage();
	connect(resetButton, SIGNAL(clicked()), m_backgroundImage, SLOT(refresh()));
//	connect(m_backgroundImage, SIGNAL(polygonSelected(QPolygon)), this, SLOT(on_polygonSelected(QPolygon)));
	connect(m_backgroundImage, SIGNAL(descriptionComplete(const QImage&, const std::list<QPolygon>&)), this, SLOT(on_descriptionComplete(const QImage&, const std::list<QPolygon>&)));

	QVBoxLayout *polygonLayout = new QVBoxLayout(m_polygonsWidget);
	mainLayout->addWidget(m_backgroundImage);
	mainLayout->addWidget(m_polygonsWidget);
	static_cast<QVBoxLayout*>(m_polygonsWidget->layout())->addWidget(resetButton, false, Qt::AlignTop);
	master->setLayout(mainLayout);



	master->show();
}

ImageDescriptor::~ImageDescriptor()
{
	delete master;
}

void ImageDescriptorImage::refresh()
{
	Mat img;
	m_cap>>img;
	m_image = iez::WindowManager::Mat2QImage(img);
	refresh(m_image);
}

void ImageDescriptorImage::refresh(const QImage &image)
{
	m_image = image;
	setFixedSize(image.width(), image.height());
	setPixmap(QPixmap::fromImage(image));
}


//void ImageDescriptor::on_polygonSelected(QPolygon polygon)
//{
//	foreach (QPoint pt, polygon) {
//		qDebug("%d %d", pt.x(), pt.y());
//	}
//	addPolygon(polygon);
//}
//
//void ImageDescriptor::addPolygon(QPolygon polygon)
//{
////	m_polygonList.push_back(polygon);
////	QLabel* lbl = new QLabel("Polygon");
////	static_cast<QVBoxLayout*>(m_polygonsWidget->layout())->addWidget(lbl, false, Qt::AlignTop);
//}
void ImageDescriptor::on_descriptionComplete(const QImage& image, const std::list<QPolygon> &polygonList)
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

}// namespace iez


