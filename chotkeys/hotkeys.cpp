/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#include "hotkeys.h"

#include <KGlobalAccel>
#include <QAction>
#include <QCoreApplication>
#include <QDebug>

Hotkeys::Hotkeys(QObject *parent)
    : QObject(parent)
{
}

Hotkeys::~Hotkeys()
{
    for (QAction *action : std::as_const(m_actions))
        KGlobalAccel::self()->removeAllShortcuts(action);
    qDeleteAll(m_actions);
}

void Hotkeys::registerKey(QKeySequence keySequence)
{
    if (keySequence.isEmpty())
        return;

    QAction *action = new QAction(this);
    action->setObjectName(keySequence.toString());
    action->setProperty("componentName", QCoreApplication::applicationName());
    action->setProperty("componentDisplayName", QStringLiteral("Cutefish Hotkeys"));

    KGlobalAccel::self()->setShortcut(action, {keySequence}, KGlobalAccel::NoAutoloading);

    connect(action, &QAction::triggered, this, [this, keySequence]() {
        emit pressed(keySequence);
    });

    m_actions.append(action);
    qDebug() << "Registered global shortcut:" << keySequence.toString();
}

void Hotkeys::registerKey(quint32 keycode)
{
    // X11 raw keycode registration is not supported on Wayland.
    Q_UNUSED(keycode)
}
