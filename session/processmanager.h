/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include <QEventLoop>
#include <QMap>

class Application;

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(Application *app, QObject *parent = nullptr);
    ~ProcessManager();

    void start();
    void logout();

    void startWindowManager();
    void startDesktopProcess();
    void startDaemonProcess();
    void loadAutoStartProcess();

private:
    void onKWinReady();

    Application *m_app;
    QMap<QString, QProcess *> m_systemProcess;
    QMap<QString, QProcess *> m_autoStartProcess;
};

#endif // PROCESSMANAGER_H
