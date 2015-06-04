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

#include <QVariant>
#include <QMutex>
#include <QHash>
#include <QDialog>

class QSettings;
class QString;

namespace iez {

class Settings {
public:
	static Settings *instance();
	void setValue(const QString &key, const QVariant &value);
	QVariant value(const QString &key) const;
	QStringList allKeys() const;
private:
	static const QList<QPair<QString, QVariant>> defaultsList;

	Settings();
	~Settings();
	Settings(Settings &);

	void setDefaults();

	mutable QMutex m_mutex;
	QSettings *m_settings;
};

class SettingsDialog : public QDialog {
Q_OBJECT
public:
	SettingsDialog();
	~SettingsDialog(){}
private:
	Settings *m_settings;
private slots:
	void on_accepted();
};

}
