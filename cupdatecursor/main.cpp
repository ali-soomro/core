/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

// On Wayland the cursor theme is owned by the compositor (KWin).
// We write the chosen theme/size to kcminputrc and ask KWin to reconfigure.
// The X11/Xcursor implementation has been removed.

#include <QGuiApplication>
#include <QDBusInterface>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>

int main(int argc, char *argv[])
{
    QGuiApplication::setDesktopSettingsAware(false);
    QGuiApplication app(argc, argv);

    if (argc != 3) {
        qWarning() << "Usage: cupdatecursor <theme> <size>";
        return 1;
    }

    const QString theme = QFile::decodeName(argv[1]);
    const int size = QFile::decodeName(argv[2]).toInt();

    // Persist to KWin's input config so the setting survives restart.
    QSettings settings(
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/kcminputrc"),
        QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Mouse"));
    settings.setValue(QStringLiteral("cursorTheme"), theme);
    settings.setValue(QStringLiteral("cursorSize"), size);
    settings.endGroup();
    settings.sync();

    // Set environment variables so newly spawned processes pick them up.
    qputenv("XCURSOR_THEME", theme.toLocal8Bit());
    qputenv("XCURSOR_SIZE", QByteArray::number(size));

    // Ask KWin to reload its configuration (picks up the new cursor immediately).
    QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"),
                        QDBusConnection::sessionBus());
    if (kwin.isValid())
        kwin.asyncCall(QStringLiteral("reconfigure"));
    else
        qWarning() << "cupdatecursor: KWin D-Bus not available; cursor updated in config only";

    return 0;
}
