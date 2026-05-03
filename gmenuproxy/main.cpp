/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <QGuiApplication>
#include <QSessionManager>

#include <KWindowSystem>

#include "menuproxy.h"

int main(int argc, char **argv)
{
    // gmenuproxy is X11-only, skip on Wayland
    if (!qgetenv("WAYLAND_DISPLAY").isEmpty()) {
        // Running on Wayland, exit gracefully
        return 0;
    }

    qputenv("QT_QPA_PLATFORM", "xcb");

    QGuiApplication::setDesktopSettingsAware(false);

    QGuiApplication app(argc, argv);

    if (!KWindowSystem::isPlatformX11()) {
        qFatal("qdbusmenuproxy is only useful on X11. Aborting");
    }

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    app.setQuitOnLastWindowClosed(false);

    MenuProxy proxy;

    return app.exec();
}
