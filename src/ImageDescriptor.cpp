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
			}

			emit polygonSelected(polygon);
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
	connect(m_backgroundImage, SIGNAL(polygonSelected(QPolygon)), this, SLOT(on_polygonSelected(QPolygon)));

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
	image = iez::CWindowManager::Mat2QImage(img);
	refresh(image);
}

void ImageDescriptorImage::refresh(const QImage &image)
{
	m_image = image;
	QFileDialog *dialog = new QFileDialog(this, "a", "/");
	dialog->show();
	connect(dialog, SIGNAL(fileSelected(const QString &)), this, SLOT(on_fileSelected(const QString &)));
	setFixedSize(image.width(), image.height());
	setPixmap(QPixmap::fromImage(image));
}

void ImageDescriptorImage::on_fileSelected(const QString &file)
{
	qDebug("%s", file.toAscii().constData());
}
void ImageDescriptor::on_polygonSelected(QPolygon polygon)
{
	foreach (QPoint pt, polygon) {
		qDebug("%d %d", pt.x(), pt.y());
	}
	addPolygon(polygon);
}

void ImageDescriptor::addPolygon(QPolygon polygon)
{
	m_polygonsList.push_back(polygon);
	QLabel* lbl = new QLabel("Polygon");
	static_cast<QVBoxLayout*>(m_polygonsWidget->layout())->addWidget(lbl, false, Qt::AlignTop);
}

}// namespace iez


