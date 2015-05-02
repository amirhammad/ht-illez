#pragma once
#include <opencv2/opencv.hpp>
#include <QtCore/QObject>
#include <QtGui/QLabel>
#include <QtGui/qevent.h>
#include <QtCore/qmutex.h>

namespace iez {

class Window : public QLabel {
	Q_OBJECT
public:
	Window(){};

private:
	void keyPressEvent(QKeyEvent *keyEvent) {
		emit keyPressed(keyEvent);
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
	static WindowManager * getInstance()
	{
		static WindowManager *instance = new WindowManager();
		return instance;
	}
	static void destroy()
	{
		if (getInstance() != 0) {
			getInstance()->deleteLater();
		}
	}

private:
	// Dont forget to declare these two. You want to make sure they
	// are unaccessable otherwise you may accidently get copies of
	// your singleton appearing.
	WindowManager(WindowManager const&); // Don't Implement
	void operator=(WindowManager const&); // Don't implement

	WindowManager();
	~WindowManager();


public:
	void imShow(const QString name, const cv::Mat &image);
	void imShow(const QString name, const QImage& image);

	static cv::Mat QImage2Mat(QImage const& src);
	static QImage Mat2QImage(cv::Mat const& src);

private:

	struct imShowMapData {
		Window *widget;
		QImage image;
	};
	QHash<QString, struct imShowMapData > m_imShowMap;

	QMutex m_mutex;

public slots:
	void on_imShow(const QString name);

signals:
	void keyPressed(QKeyEvent *keyEvent);
};
}
