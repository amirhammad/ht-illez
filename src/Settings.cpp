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
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace iez {

const QList<QPair<QString, QVariant>> Settings::defaultsList ({
	{"PoseRecognition::m_poseCount", 12},
	{"HandTracker::m_bDebug", false}
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
		if (!m_settings->contains(pair.first)) {
			m_settings->setValue(pair.first, pair.second);
		}
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
	return QVariant(m_settings->value(key));
}

SettingsDialog::SettingsDialog()
:	QDialog(0)
{
	setWindowTitle("HandTrackerApp::Settings");
	m_settings = Settings::instance();
	QGridLayout * mainLayout = new QGridLayout(this);
	QStringList keyList = m_settings->allKeys();

	for (int i = 0; i < keyList.size(); i++) {
		const QString &key = keyList[i];
		QVariant val = m_settings->value(key);

		if (!val.canConvert<QString>()) {continue;}

		QLabel *keyLabel = new QLabel(key, this);
		keyLabel->setAlignment(Qt::AlignRight);
		QLineEdit *valueTextEdit = new QLineEdit(val.toString(), this);

		mainLayout->addWidget(keyLabel, i, 0);
		mainLayout->addWidget(valueTextEdit, i, 1);
	}

	QPushButton *okPushButton = new QPushButton("ok", this);
	QObject::connect(okPushButton, SIGNAL(clicked()), this, SLOT(on_accepted()));

	QPushButton *cancelPushButton = new QPushButton("cancel", this);
	QObject::connect(cancelPushButton, SIGNAL(clicked()), this, SIGNAL(rejected()));

	mainLayout->addWidget(okPushButton, keyList.size(), 0);
	mainLayout->addWidget(cancelPushButton, keyList.size(), 1);
	QDialog::setLayout(mainLayout);
	show();
}

void SettingsDialog::on_accepted()
{
	QGridLayout * layout = dynamic_cast<QGridLayout*>(QDialog::layout());

	// last row is ok|cancel
	const int settingCount = layout->rowCount() - 1;
	for (int i = 0; i < settingCount; i++) {
		QLabel *keyLabel = dynamic_cast<QLabel *>(layout->itemAtPosition(i, 0)->widget());
		Q_ASSERT(keyLabel);
		QString key = keyLabel->text();

		QLineEdit *valueLineEdit = dynamic_cast<QLineEdit *>(layout->itemAtPosition(i, 1)->widget());
		Q_ASSERT(valueLineEdit);
		QString value = valueLineEdit->text();

		m_settings->setValue(key, value);
	}
	emit accepted();
}

QStringList Settings::allKeys() const
{
	return m_settings->allKeys();
}
}
