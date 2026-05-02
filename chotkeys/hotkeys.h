/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#ifndef HOTKEYS_H
#define HOTKEYS_H

#include <QObject>
#include <QKeySequence>
#include <QList>

class QAction;

// Wayland-compatible global shortcut registration via KGlobalAccel.
// The X11/XCB key-grabbing implementation has been replaced entirely.
class Hotkeys : public QObject
{
    Q_OBJECT

public:
    explicit Hotkeys(QObject *parent = nullptr);
    ~Hotkeys() override;

    void registerKey(QKeySequence keySequence);
    // Raw keycode registration was X11-specific; kept as no-op for source compatibility.
    void registerKey(quint32 keycode);

signals:
    void pressed(QKeySequence keySeq);
    // 'released' is not reliably available via KGlobalAccel; kept for API compat.
    void released(QKeySequence keySeq);

private:
    QList<QAction *> m_actions;
};

#endif // HOTKEYS_H
