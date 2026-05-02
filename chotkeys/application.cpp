/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#include "application.h"
#include "hotkeys.h"

#include <QProcess>
#include <QDebug>

Application::Application(QObject *parent)
    : QObject(parent)
    , m_hotKeys(new Hotkeys(this))
{
    setupShortcuts();
    connect(m_hotKeys, &Hotkeys::pressed, this, &Application::onPressed);
}

void Application::setupShortcuts()
{
    // Qt6: use Qt::KeyboardModifier enums directly with operator|
    m_hotKeys->registerKey(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Delete));
    m_hotKeys->registerKey(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A));
    m_hotKeys->registerKey(QKeySequence(Qt::META | Qt::Key_L));
    m_hotKeys->registerKey(QKeySequence(Qt::META | Qt::Key_Space)); // launcher (was raw keycode 647)
}

void Application::onPressed(QKeySequence keySeq)
{
    const QString seq = keySeq.toString();

    if (seq == QLatin1String("Ctrl+Alt+Del"))
        QProcess::startDetached(QStringLiteral("cutefish-shutdown"), {});

    if (seq == QLatin1String("Meta+L"))
        QProcess::startDetached(QStringLiteral("cutefish-screenlocker"), {});

    if (seq == QLatin1String("Ctrl+Alt+A"))
        QProcess::startDetached(QStringLiteral("cutefish-screenshot"), {});

    if (seq == QLatin1String("Meta+Space"))
        QProcess::startDetached(QStringLiteral("cutefish-launcher"), {});
}
