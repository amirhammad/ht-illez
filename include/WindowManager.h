#pragma once
#include <opencv2/opencv.hpp>
#include <QtGui/qlabel.h>
#include <QtCore/qmutex.h>
#include "qcustomplot.h"

namespace iez {


class S
{
    public:
        static S& getInstance()
        {
            static S    instance; // Guaranteed to be destroyed.
                                  // Instantiated on first use.
            return instance;
        }
    private:
        S() {};                   // Constructor? (the {} brackets) are needed here.
        // Dont forget to declare these two. You want to make sure they
        // are unaccessable otherwise you may accidently get copies of
        // your singleton appearing.
        S(S const&);              // Don't Implement
        void operator=(S const&); // Don't implement
};

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

class WindowManager: public QObject {
	Q_OBJECT
public:
	static WindowManager& getInstance()
	{
		static WindowManager    instance; // Guaranteed to be destroyed.
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
	void imShow(const char *str, const cv::Mat &image);
	void imShow(const char *name, const QImage& image);
	void plot(const char *name,	const QVector<double> &x, const QVector<double> &y);

	static cv::Mat QImage2Mat(QImage const& src);
	static QImage Mat2QImage(cv::Mat const& src);

private:

	struct imShowMapData {
		CWindow *widget;
		QImage image;
	};
	std::map<const char *, struct imShowMapData > m_imShowMap;

	struct plotMapData {
		CWindow *widget;
		QVector<double> x;
		QVector<double> y;
		QCustomPlot *customPlot;
	};
	std::map<const char *, struct plotMapData> m_plotMap;

	QMutex m_mutex;

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
