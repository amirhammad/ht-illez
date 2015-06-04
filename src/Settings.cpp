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

#include "Settings.h"

#include <QSettings>
#include <QMutex>
#include <QSet>
#include <QDebug>

namespace iez {

const QList<QPair<QString, QVariant>> Settings::defaultsList ({
	{"PoseRecognition::m_poseCount", 12}
});

Settings::Settings()
{
	m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "HandTrackerFriends", "HandTrackerApp");

	setDefaults();
}

Settings::~Settings()
{

}

void Settings::setDefaults()
{
	foreach (auto pair, defaultsList) {
		m_defaults[pair.first] = pair.second;
	};
}

Settings *Settings::instance()
{
	static Settings settings;
	return &settings;
}

void Settings::setValue(const QString &key, const QVariant &value)
{
	QMutexLocker locker(&m_mutex);
	m_settings->setValue(key, value);
}

QVariant Settings::value(const QString &key) const
{
	QMutexLocker locker(&m_mutex);
	return QVariant(m_settings->value(key, m_defaults[key]));
}

}
