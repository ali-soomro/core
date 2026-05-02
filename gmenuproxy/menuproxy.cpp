/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "menuproxy.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QSettings>
#include <QDebug>

#include <KDirWatch>
#include <KWindowSystem>

#include "window.h"

// X11-only window tracking support.
#ifdef Q_OS_LINUX
#  include <xcb/xcb.h>
#  include <KX11Extras>
#  include <KWindowInfo>
#  include <netwm_def.h>
static xcb_connection_t *s_xcbConnection = nullptr;
#endif

static const QString s_ourServiceName    = QStringLiteral("org.kde.plasma.gmenu_dbusmenu_proxy");
static const QString s_dbusMenuRegistrar = QStringLiteral("com.canonical.AppMenu.Registrar");

static const QByteArray s_gtkUniqueBusName          = QByteArrayLiteral("_GTK_UNIQUE_BUS_NAME");
static const QByteArray s_gtkApplicationObjectPath  = QByteArrayLiteral("_GTK_APPLICATION_OBJECT_PATH");
static const QByteArray s_unityObjectPath           = QByteArrayLiteral("_UNITY_OBJECT_PATH");
static const QByteArray s_gtkWindowObjectPath       = QByteArrayLiteral("_GTK_WINDOW_OBJECT_PATH");
static const QByteArray s_gtkMenuBarObjectPath      = QByteArrayLiteral("_GTK_MENUBAR_OBJECT_PATH");
static const QByteArray s_gtkAppMenuObjectPath      = QByteArrayLiteral("_GTK_APP_MENU_OBJECT_PATH");
static const QByteArray s_kdeNetWmAppMenuServiceName = QByteArrayLiteral("_KDE_NET_WM_APPMENU_SERVICE_NAME");
static const QByteArray s_kdeNetWmAppMenuObjectPath  = QByteArrayLiteral("_KDE_NET_WM_APPMENU_OBJECT_PATH");

static const QString s_gtkModules      = QStringLiteral("gtk-modules");
static const QString s_appMenuGtkModule = QStringLiteral("appmenu-gtk-module");

MenuProxy::MenuProxy()
    : QObject()
    , m_serviceWatcher(new QDBusServiceWatcher(this))
    , m_gtk2RcWatch(new KDirWatch(this))
    , m_writeGtk2SettingsTimer(new QTimer(this))
    , m_isX11(QGuiApplication::platformName() == QLatin1String("xcb"))
{
#ifdef Q_OS_LINUX
    if (m_isX11) {
        s_xcbConnection = xcb_connect(nullptr, nullptr);
        if (xcb_connection_has_error(s_xcbConnection)) {
            s_xcbConnection = nullptr;
            m_isX11 = false;
        }
    }
#endif

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration);
    m_serviceWatcher->addWatchedService(s_dbusMenuRegistrar);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &) {
        qDebug() << "Global menu service became available, starting";
        init();
    });
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &) {
        qDebug() << "Global menu service disappeared, cleaning up";
        teardown();
    });

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(s_dbusMenuRegistrar)) {
        qDebug() << "Global menu service is running, starting right away";
        init();
    } else {
        enableGtkSettings(false);
    }

    m_writeGtk2SettingsTimer->setSingleShot(true);
    m_writeGtk2SettingsTimer->setInterval(1000);
    connect(m_writeGtk2SettingsTimer, &QTimer::timeout, this, &MenuProxy::writeGtk2Settings);

    auto startGtk2Timer = [this] {
        if (!m_writeGtk2SettingsTimer->isActive())
            m_writeGtk2SettingsTimer->start();
    };
    connect(m_gtk2RcWatch, &KDirWatch::created, this, startGtk2Timer);
    connect(m_gtk2RcWatch, &KDirWatch::dirty,   this, startGtk2Timer);
    m_gtk2RcWatch->addFile(gtkRc2Path());
}

MenuProxy::~MenuProxy()
{
    teardown();
#ifdef Q_OS_LINUX
    if (s_xcbConnection) {
        xcb_disconnect(s_xcbConnection);
        s_xcbConnection = nullptr;
    }
#endif
}

bool MenuProxy::init()
{
    if (!QDBusConnection::sessionBus().registerService(s_ourServiceName)) {
        qDebug() << "Failed to register DBus service" << s_ourServiceName;
        return false;
    }

    enableGtkSettings(true);

    // Window tracking is X11-only; on Wayland the GTK module handles menu
    // export through D-Bus without needing XCB property access.
    if (m_isX11) {
#ifdef Q_OS_LINUX
        connect(KX11Extras::self(), &KX11Extras::windowAdded,   this, &MenuProxy::onWindowAdded);
        connect(KX11Extras::self(), &KX11Extras::windowRemoved, this, &MenuProxy::onWindowRemoved);

        const auto windows = KX11Extras::windows();
        for (WId id : windows)
            onWindowAdded(id);
#endif
    }

    return true;
}

void MenuProxy::teardown()
{
    enableGtkSettings(false);
    QDBusConnection::sessionBus().unregisterService(s_ourServiceName);

    if (m_isX11) {
#ifdef Q_OS_LINUX
        disconnect(KX11Extras::self(), &KX11Extras::windowAdded,   this, &MenuProxy::onWindowAdded);
        disconnect(KX11Extras::self(), &KX11Extras::windowRemoved, this, &MenuProxy::onWindowRemoved);
#endif
    }

    qDeleteAll(m_windows);
    m_windows.clear();
}

void MenuProxy::enableGtkSettings(bool enable)
{
    m_enabled = enable;
    writeGtk2Settings();
    writeGtk3Settings();
}

QString MenuProxy::gtkRc2Path()
{
    return QDir::homePath() + QLatin1String("/.gtkrc-2.0");
}

QString MenuProxy::gtk3SettingsIniPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + QLatin1String("/gtk-3.0/settings.ini");
}

void MenuProxy::writeGtk2Settings()
{
    QFile rcFile(gtkRc2Path());
    if (!rcFile.exists())
        return;

    if (!rcFile.open(QIODevice::ReadWrite | QIODevice::Text))
        return;

    QByteArray content;
    QStringList gtkModules;

    while (!rcFile.atEnd()) {
        const QByteArray rawLine = rcFile.readLine();
        const QString line = QString::fromUtf8(rawLine.trimmed());

        if (!line.startsWith(s_gtkModules)) {
            content += rawLine;
            continue;
        }

        const int eqIdx = line.indexOf(QLatin1Char('='));
        if (eqIdx >= 1)
            gtkModules = line.mid(eqIdx + 1).split(QLatin1Char(':'), Qt::SkipEmptyParts);
        break;
    }

    addOrRemoveAppMenuGtkModule(gtkModules);
    if (!gtkModules.isEmpty())
        content += QStringLiteral("%1=%2").arg(s_gtkModules, gtkModules.join(QLatin1Char(':'))).toUtf8();

    m_gtk2RcWatch->stopScan();
    rcFile.resize(0);
    rcFile.write(content);
    rcFile.close();
    m_gtk2RcWatch->startScan();
}

void MenuProxy::writeGtk3Settings()
{
    QSettings cfg(gtk3SettingsIniPath(), QSettings::IniFormat);
    cfg.beginGroup(QStringLiteral("Settings"));

    QStringList gtkModules = cfg.value(QStringLiteral("gtk-modules")).toString().split(QLatin1Char(':'));
    addOrRemoveAppMenuGtkModule(gtkModules);

    if (!gtkModules.isEmpty())
        cfg.setValue(QStringLiteral("gtk-modules"), gtkModules.join(QLatin1Char(':')));
    else
        cfg.remove(QStringLiteral("gtk-modules"));

    if (m_enabled)
        cfg.setValue(QStringLiteral("gtk-shell-shows-menubar"), 1);
    else
        cfg.remove(QStringLiteral("gtk-shell-shows-menubar"));

    cfg.sync();
}

void MenuProxy::addOrRemoveAppMenuGtkModule(QStringList &list)
{
    if (m_enabled && !list.contains(s_appMenuGtkModule))
        list.append(s_appMenuGtkModule);
    else if (!m_enabled)
        list.removeAll(s_appMenuGtkModule);
}

void MenuProxy::onWindowAdded(WId id)
{
    if (!m_isX11 || m_windows.contains(id))
        return;

#ifdef Q_OS_LINUX
    KWindowInfo info(id, NET::WMWindowType);
    NET::WindowType wType = info.windowType(NET::NormalMask | NET::DesktopMask | NET::DockMask |
                                            NET::ToolbarMask | NET::MenuMask | NET::DialogMask |
                                            NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask |
                                            NET::SplashMask);
    if (wType != NET::Normal)
        return;
#endif

    const QString serviceName = QString::fromUtf8(getWindowPropertyString(id, s_gtkUniqueBusName));
    if (serviceName.isEmpty())
        return;

    const QString applicationObjectPath = QString::fromUtf8(getWindowPropertyString(id, s_gtkApplicationObjectPath));
    const QString unityObjectPath       = QString::fromUtf8(getWindowPropertyString(id, s_unityObjectPath));
    const QString windowObjectPath      = QString::fromUtf8(getWindowPropertyString(id, s_gtkWindowObjectPath));
    const QString applicationMenuPath   = QString::fromUtf8(getWindowPropertyString(id, s_gtkAppMenuObjectPath));
    const QString menuBarObjectPath     = QString::fromUtf8(getWindowPropertyString(id, s_gtkMenuBarObjectPath));

    if (applicationMenuPath.isEmpty() && menuBarObjectPath.isEmpty())
        return;

    Window *window = new Window(serviceName);
    window->setWinId(id);
    window->setApplicationObjectPath(applicationObjectPath);
    window->setUnityObjectPath(unityObjectPath);
    window->setWindowObjectPath(windowObjectPath);
    window->setApplicationMenuObjectPath(applicationMenuPath);
    window->setMenuBarObjectPath(menuBarObjectPath);
    m_windows.insert(id, window);

    connect(window, &Window::requestWriteWindowProperties, this, [this, window] {
        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuServiceName, s_ourServiceName.toUtf8());
        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuObjectPath,  window->proxyObjectPath().toUtf8());
    });
    connect(window, &Window::requestRemoveWindowProperties, this, [this, window] {
        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuServiceName, {});
        writeWindowProperty(window->winId(), s_kdeNetWmAppMenuObjectPath,  {});
    });

    window->init();
}

void MenuProxy::onWindowRemoved(WId id)
{
    delete m_windows.take(id);
}

QByteArray MenuProxy::getWindowPropertyString(WId id, const QByteArray &name)
{
#ifdef Q_OS_LINUX
    if (!m_isX11 || !s_xcbConnection)
        return {};

    // Resolve atom
    static QHash<QByteArray, xcb_atom_t> s_atoms;
    auto resolveAtom = [&](const QByteArray &n) -> xcb_atom_t {
        auto it = s_atoms.find(n);
        if (it != s_atoms.end())
            return it.value();
        auto cookie = xcb_intern_atom(s_xcbConnection, false, n.length(), n.constData());
        QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> reply(
            xcb_intern_atom_reply(s_xcbConnection, cookie, nullptr));
        xcb_atom_t atom = reply ? reply->atom : XCB_ATOM_NONE;
        if (atom != XCB_ATOM_NONE)
            s_atoms.insert(n, atom);
        return atom;
    };

    xcb_atom_t atom = resolveAtom(name);
    xcb_atom_t utf8Atom = resolveAtom(QByteArrayLiteral("UTF8_STRING"));
    if (atom == XCB_ATOM_NONE || utf8Atom == XCB_ATOM_NONE)
        return {};

    static const long MAX_PROP_SIZE = 10000;
    auto cookie = xcb_get_property(s_xcbConnection, false, id, atom, utf8Atom, 0, MAX_PROP_SIZE);
    QScopedPointer<xcb_get_property_reply_t, QScopedPointerPodDeleter> reply(
        xcb_get_property_reply(s_xcbConnection, cookie, nullptr));
    if (!reply || reply->type != utf8Atom || reply->format != 8 || reply->value_len == 0)
        return {};

    const char *data = static_cast<const char *>(xcb_get_property_value(reply.data()));
    int len = reply->value_len;
    return data ? QByteArray(data, data[len - 1] ? len : len - 1) : QByteArray{};
#else
    Q_UNUSED(id) Q_UNUSED(name)
    return {};
#endif
}

void MenuProxy::writeWindowProperty(WId id, const QByteArray &name, const QByteArray &value)
{
#ifdef Q_OS_LINUX
    if (!m_isX11 || !s_xcbConnection)
        return;

    static QHash<QByteArray, xcb_atom_t> s_atoms;
    auto resolveAtom = [&](const QByteArray &n) -> xcb_atom_t {
        auto it = s_atoms.find(n);
        if (it != s_atoms.end())
            return it.value();
        auto cookie = xcb_intern_atom(s_xcbConnection, false, n.length(), n.constData());
        QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> reply(
            xcb_intern_atom_reply(s_xcbConnection, cookie, nullptr));
        xcb_atom_t atom = reply ? reply->atom : XCB_ATOM_NONE;
        if (atom != XCB_ATOM_NONE)
            s_atoms.insert(n, atom);
        return atom;
    };

    xcb_atom_t atom = resolveAtom(name);
    if (atom == XCB_ATOM_NONE)
        return;

    if (value.isEmpty())
        xcb_delete_property(s_xcbConnection, id, atom);
    else
        xcb_change_property(s_xcbConnection, XCB_PROP_MODE_REPLACE, id, atom,
                            XCB_ATOM_STRING, 8, value.length(), value.constData());
#else
    Q_UNUSED(id) Q_UNUSED(name) Q_UNUSED(value)
#endif
}
