/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     Reion Wong <reionwong@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mousemanager.h"
#include "mouseadaptor.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QSettings>
#include <QStandardPaths>

Mouse::Mouse(QObject *parent)
    : QObject(parent)
{
    new MouseAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Mouse"), this);

    // Load persisted settings.
    QSettings cfg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                      + QStringLiteral("/kcminputrc"),
                  QSettings::IniFormat);
    cfg.beginGroup(QStringLiteral("Mouse"));
    m_leftHanded        = cfg.value(QStringLiteral("LeftHanded"),        false).toBool();
    m_acceleration      = cfg.value(QStringLiteral("AccelerationFlat"),  false).toBool();
    m_naturalScroll     = cfg.value(QStringLiteral("NaturalScroll"),     false).toBool();
    m_pointerAcceleration = cfg.value(QStringLiteral("PointerAcceleration"), 0.0).toReal();
}

bool Mouse::leftHanded() const { return m_leftHanded; }
void Mouse::setLeftHanded(bool enabled)
{
    if (m_leftHanded == enabled) return;
    m_leftHanded = enabled;
    applyConfig();
    emit leftHandedChanged();
}

bool Mouse::acceleration() const { return m_acceleration; }
void Mouse::setAcceleration(bool enabled)
{
    if (m_acceleration == enabled) return;
    m_acceleration = enabled;
    applyConfig();
    emit accelerationChanged();
}

bool Mouse::naturalScroll() const { return m_naturalScroll; }
void Mouse::setNaturalScroll(bool enabled)
{
    if (m_naturalScroll == enabled) return;
    m_naturalScroll = enabled;
    applyConfig();
    emit naturalScrollChanged();
}

qreal Mouse::pointerAcceleration() const { return m_pointerAcceleration; }
void Mouse::setPointerAcceleration(qreal value)
{
    if (qFuzzyCompare(m_pointerAcceleration, value)) return;
    m_pointerAcceleration = value;
    applyConfig();
    emit pointerAccelerationChanged();
}

void Mouse::applyConfig()
{
    QSettings cfg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                      + QStringLiteral("/kcminputrc"),
                  QSettings::IniFormat);
    cfg.beginGroup(QStringLiteral("Mouse"));
    cfg.setValue(QStringLiteral("LeftHanded"),           m_leftHanded);
    cfg.setValue(QStringLiteral("AccelerationFlat"),     m_acceleration);
    cfg.setValue(QStringLiteral("NaturalScroll"),        m_naturalScroll);
    cfg.setValue(QStringLiteral("PointerAcceleration"),  m_pointerAcceleration);
    cfg.sync();

    QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"),
                        QDBusConnection::sessionBus());
    if (kwin.isValid())
        kwin.asyncCall(QStringLiteral("reconfigureInput"));
}
