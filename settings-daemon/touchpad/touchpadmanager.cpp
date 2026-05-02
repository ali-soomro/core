#include "touchpadmanager.h"
#include "touchpadadaptor.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QSettings>
#include <QStandardPaths>

TouchpadManager::TouchpadManager(QObject *parent)
    : QObject(parent)
{
    new TouchpadAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Touchpad"), this);

    // On Wayland the compositor manages libinput directly; we persist settings
    // in kcminputrc and ask KWin to reload them.
    QSettings cfg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                      + QStringLiteral("/kcminputrc"),
                  QSettings::IniFormat);
    cfg.beginGroup(QStringLiteral("Touchpad"));
    m_enabled             = cfg.value(QStringLiteral("Enabled"),            true).toBool();
    m_tapToClick          = cfg.value(QStringLiteral("TapToClick"),         false).toBool();
    m_naturalScroll       = cfg.value(QStringLiteral("NaturalScroll"),      false).toBool();
    m_pointerAcceleration = cfg.value(QStringLiteral("PointerAcceleration"),0.0).toReal();

    // Assume a touchpad is available on any machine running this daemon.
    m_available = true;
}

bool TouchpadManager::available() const { return m_available; }

bool TouchpadManager::enabled() const { return m_enabled; }
void TouchpadManager::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    applyConfig();
}

bool TouchpadManager::tapToClick() const { return m_tapToClick; }
void TouchpadManager::setTapToClick(bool value)
{
    if (m_tapToClick == value) return;
    m_tapToClick = value;
    applyConfig();
}

bool TouchpadManager::naturalScroll() const { return m_naturalScroll; }
void TouchpadManager::setNaturalScroll(bool value)
{
    if (m_naturalScroll == value) return;
    m_naturalScroll = value;
    applyConfig();
}

qreal TouchpadManager::pointerAcceleration() const { return m_pointerAcceleration; }
void TouchpadManager::setPointerAcceleration(qreal value)
{
    if (qFuzzyCompare(m_pointerAcceleration, value)) return;
    m_pointerAcceleration = value;
    applyConfig();
}

void TouchpadManager::applyConfig()
{
    QSettings cfg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                      + QStringLiteral("/kcminputrc"),
                  QSettings::IniFormat);
    cfg.beginGroup(QStringLiteral("Touchpad"));
    cfg.setValue(QStringLiteral("Enabled"),             m_enabled);
    cfg.setValue(QStringLiteral("TapToClick"),          m_tapToClick);
    cfg.setValue(QStringLiteral("NaturalScroll"),       m_naturalScroll);
    cfg.setValue(QStringLiteral("PointerAcceleration"), m_pointerAcceleration);
    cfg.sync();

    QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"),
                        QDBusConnection::sessionBus());
    if (kwin.isValid())
        kwin.asyncCall(QStringLiteral("reconfigureInput"));
}
