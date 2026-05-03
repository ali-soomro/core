/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#include "processmanager.h"
#include "application.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfoList>
#include <QFileInfo>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QTimer>
#include <QDir>

ProcessManager::ProcessManager(Application *app, QObject *parent)
    : QObject(parent)
    , m_app(app)
{
}

ProcessManager::~ProcessManager()
{
    QMapIterator<QString, QProcess *> i(m_systemProcess);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
}

void ProcessManager::start()
{
    startWindowManager();
    startDaemonProcess();
}

void ProcessManager::logout()
{
    QDBusInterface kwinIface(QStringLiteral("org.kde.KWin"),
                             QStringLiteral("/Session"),
                             QStringLiteral("org.kde.KWin.Session"),
                             QDBusConnection::sessionBus());
    if (kwinIface.isValid()) {
        kwinIface.call(QStringLiteral("aboutToSaveSession"), QStringLiteral("cutefish"));
        kwinIface.call(QStringLiteral("setState"), uint(2));
    }

    QProcess::execute(QStringLiteral("killall"), {QStringLiteral("kglobalaccel6")});

    QDBusInterface loginIface(QStringLiteral("org.freedesktop.login1"),
                              QStringLiteral("/org/freedesktop/login1/session/self"),
                              QStringLiteral("org.freedesktop.login1.Session"),
                              QDBusConnection::systemBus());
    if (loginIface.isValid())
        loginIface.call(QStringLiteral("Terminate"));

    QCoreApplication::exit(0);
}

void ProcessManager::startWindowManager()
{
    const QString wm = m_app->wayland() ? QStringLiteral("kwin_wayland") : QStringLiteral("kwin");
    QProcess *wmProcess = new QProcess(this);
    wmProcess->start(wm, {});
    m_systemProcess.insert(wm, wmProcess);

    if (m_app->wayland()) {
        // On Wayland, wait for KWin to register on D-Bus before starting desktop.
        auto *watcher = new QDBusServiceWatcher(
            QStringLiteral("org.kde.KWin"),
            QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration,
            this);
        connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, [this, watcher]() {
            qDebug() << "KWin Wayland compositor ready";
            watcher->deleteLater();
            startDesktopProcess();
        });
        // Fallback: start desktop after 10 s even if D-Bus signal never fires.
        QTimer::singleShot(10000, this, [this, watcher]() {
            if (watcher->parent()) {
                qWarning() << "KWin D-Bus timeout; starting desktop anyway";
                watcher->deleteLater();
                startDesktopProcess();
            }
        });
    } else {
        // X11: wait for WM name to appear, then start desktop.
        QEventLoop waitLoop;
        QTimer::singleShot(30000, &waitLoop, &QEventLoop::quit);
        waitLoop.exec();
        startDesktopProcess();
    }
}

void ProcessManager::startDesktopProcess()
{
    QList<QPair<QString, QStringList>> list;
    list << qMakePair(QStringLiteral("cutefish-notificationd"),    QStringList());
    list << qMakePair(QStringLiteral("cutefish-statusbar"),        QStringList());
    list << qMakePair(QStringLiteral("cutefish-dock"),             QStringList());
    list << qMakePair(QStringLiteral("cutefish-filemanager"),      QStringList(QStringLiteral("--desktop")));
    list << qMakePair(QStringLiteral("cutefish-launcher"),         QStringList());
    list << qMakePair(QStringLiteral("cutefish-powerman"),         QStringList());
    list << qMakePair(QStringLiteral("cutefish-clipboard"),        QStringList());

    if (QFile::exists(QStringLiteral("/usr/bin/cutefish-welcome")) &&
            !QFile::exists(QStringLiteral("/run/live/medium/live/filesystem.squashfs"))) {
        QSettings settings(QStringLiteral("cutefishos"), QStringLiteral("login"));
        if (!settings.value(QStringLiteral("Finished"), false).toBool())
            list << qMakePair(QStringLiteral("/usr/bin/cutefish-welcome"), QStringList());
        else
            list << qMakePair(QStringLiteral("/usr/bin/cutefish-welcome"), QStringList(QStringLiteral("-d")));
    }

    for (const auto &pair : list) {
        QProcess *process = new QProcess(this);
        process->setProcessChannelMode(QProcess::ForwardedChannels);
        process->setProgram(pair.first);
        process->setArguments(pair.second);
        process->start();
        process->waitForStarted();
        qDebug() << "Started:" << pair.first << pair.second;
        m_autoStartProcess.insert(pair.first, process);
    }

    QTimer::singleShot(100, this, &ProcessManager::loadAutoStartProcess);
}

void ProcessManager::startDaemonProcess()
{
    QList<QPair<QString, QStringList>> list;
    list << qMakePair(QStringLiteral("cutefish-settings-daemon"), QStringList());
    // xembedsniproxy removed: on Wayland apps use StatusNotifierItem directly
    // gmenuproxy is X11-only, skip on Wayland
    if (!m_app->wayland()) {
        list << qMakePair(QStringLiteral("cutefish-gmenuproxy"), QStringList());
    }
    list << qMakePair(QStringLiteral("chotkeys"),                  QStringList());

    for (const auto &pair : list) {
        QProcess *process = new QProcess(this);
        process->setProcessChannelMode(QProcess::ForwardedChannels);
        process->setProgram(pair.first);
        process->setArguments(pair.second);
        process->start();
        process->waitForStarted();
        m_autoStartProcess.insert(pair.first, process);
    }
}

void ProcessManager::loadAutoStartProcess()
{
    QStringList execList;
    const QStringList dirs = QStandardPaths::locateAll(
        QStandardPaths::GenericConfigLocation,
        QStringLiteral("autostart"),
        QStandardPaths::LocateDirectory);

    for (const QString &dir : dirs) {
        const QDir d(dir);
        for (const QString &file : d.entryList({QStringLiteral("*.desktop")})) {
            QSettings desktop(d.absoluteFilePath(file), QSettings::IniFormat);
            desktop.beginGroup(QStringLiteral("Desktop Entry"));
            if (desktop.contains(QStringLiteral("OnlyShowIn")))
                continue;
            const QString exec = desktop.value(QStringLiteral("Exec")).toString();
            if (!exec.isEmpty() && !exec.contains(QStringLiteral("gmenudbusmenuproxy")))
                execList << exec;
        }
    }

    for (const QString &exec : std::as_const(execList)) {
        QProcess *process = new QProcess(this);
        process->setProgram(exec);
        process->start();
        process->waitForStarted();
        m_autoStartProcess.insert(exec, process);
    }
}
