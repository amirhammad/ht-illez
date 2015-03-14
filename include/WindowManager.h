#pragma once
#include <opencv2/opencv.hpp>
#include <QtCore/QObject>
#include <QtGui/QLabel>
#include <QtGui/qevent.h>
#include <QtCore/qmutex.h>

class QCustomPlot;

namespace iez {

class Window : public QLabel {
	Q_OBJECT
public:
	Window(){};

private:
	void keyPressEvent(QKeyEvent *keyEvent) {
//		emit keyPressed(keyEvent);
	}
	void closeEvent() {
//		emit closed();
	}
signals:
	void keyPressed(QKeyEvent *keyEvent);
	void closed();
};

class WindowManager: public QObject {
	Q_OBJECT
public:
	static WindowManager& getInstance()
	{
		static WindowManager instance; // Guaranteed to be destroyed.
							  // Instantiated on first use.
		return instance;
	}

private:
	// Dont forget to declare these two. You want to make sure they
	// are unaccessable otherwise you may accidently get copies of
	// your singleton appearing.
	WindowManager(WindowManager const&); // Don't Implement
	void operator=(WindowManager const&); // Don't implement

	WindowManager();


public:
	void imShow(const QString name, const cv::Mat &image);
	void imShow(const QString name, const QImage& image);
	void plot(const QString name, const QVector<double> &x, const QVector<double> &y);
	void plot(const QString name, const std::vector<double> &x, const std::vector<double> &y);

	static cv::Mat QImage2Mat(QImage const& src);
	static QImage Mat2QImage(cv::Mat const& src);

private:

	struct imShowMapData {
		Window *widget;
		QImage image;
	};
	QHash<const QString, struct imShowMapData > m_imShowMap;

	struct plotMapData {
		Window *widget;
		QVector<double> x;
		QVector<double> y;
		QCustomPlot *customPlot;
	};
	QHash<const QString, struct plotMapData> m_plotMap;

	QMutex m_mutex;

public slots:
	void on_imShow(const QString name);
	void on_plot(const QString name);

//	void keyPressEvent(QKeyEvent *keyEvent) {
//		emit keyPressed(keyEvent);
//	}
//	void closeEvent() {
//		emit closed();
//	}
//signals:
//	void keyPressed(QKeyEvent *keyEvent);
//	void closed();
};
}

Q_DECLARE_METATYPE(const char*)
