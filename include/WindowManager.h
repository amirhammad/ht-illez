#pragma once
#include <opencv2/opencv.hpp>
#include <QtCore>
#include <QtGui>

namespace iez {
class CWindow : public QLabel {
	Q_OBJECT
public:
	CWindow(){};

private:
	void keyPressEvent(QKeyEvent *keyEvent) {
		emit keyPressed(keyEvent);
	}
	void closeEvent() {
		emit closed();
	}
signals:
	void keyPressed(QKeyEvent *keyEvent);
	void closed();
};

class CWindowManager: public QObject {
	Q_OBJECT
public:
	CWindowManager();
//	virtual ~CWindowManager(){}
	void imShow(const char *str, const cv::Mat &image);
	void plot(const char *name,	const QVector<double> &x, const QVector<double> &y);

	std::map<const char *, std::pair<CWindow*, QImage> > m_imShowMap;
	QMutex m_mutex;
	struct plotMapData {
		CWindow *widget;
		QVector<double> x;
		QVector<double> y;
	};
	std::map<const char *, struct plotMapData> m_plotMap;

	static cv::Mat QImage2Mat(QImage const& src);
	static QImage Mat2QImage(cv::Mat const& src);

public slots:
	void on_imShow(const char * name);
	void on_plot(const char *name);

	void keyPressEvent(QKeyEvent *keyEvent) {
		emit keyPressed(keyEvent);
	}
	void closeEvent() {
		emit closed();
	}
signals:
	void keyPressed(QKeyEvent *keyEvent);
	void closed();
};
}
Q_DECLARE_METATYPE(const char*)
