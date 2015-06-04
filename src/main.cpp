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


#include "main.h"
#include "MainWindow.h"
#include "Settings.h"

#include <QApplication>
#include <chrono>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// Initialize random number generator
	std::srand(std::time(0));
	iez::Starter *starter = new iez::Starter();
	QObject::connect(starter, SIGNAL(destroyed()), &app, SLOT(quit()));

	return QApplication::exec();
}


iez::Starter::Starter()
:	m_mainWindow(0)
{
	iez::SettingsDialog *settingsDialog = new iez::SettingsDialog();
	QObject::connect(settingsDialog, SIGNAL(rejected()), settingsDialog, SLOT(deleteLater()));
	QObject::connect(settingsDialog, SIGNAL(accepted()), settingsDialog, SLOT(deleteLater()));
	QObject::connect(settingsDialog, SIGNAL(rejected()), this, SLOT(deleteLater()));
	QObject::connect(settingsDialog, SIGNAL(accepted()), this, SLOT(on_settingsLoaded()));

}

iez::Starter::~Starter()
{

}

void iez::Starter::on_settingsLoaded()
{
	m_mainWindow = new MainWindow();
	QObject::connect(m_mainWindow, SIGNAL(destroyed()), this, SLOT(deleteLater()));
}
