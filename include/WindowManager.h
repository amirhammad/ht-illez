/*
 * This file is part of the project HandTrackerApp - ht-illez
 *
 * Copyright (C) 2015 Amir Hammad <amir.hammad@hotmail.com>
 *
 *
 * HandTrackerApp - ht-illez is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
	Window(){}

private:
	void keyPressEvent(QKeyEvent *keyEvent) {
		emit keyPressed(keyEvent);
	}
signals:
	void keyPressed(QKeyEvent *keyEvent);
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

private slots:
	void on_imShow(const QString name);
	void on_keyPressed(QKeyEvent *);
signals:
	void keyPressed(int);
};
}
