# core — Cutefish Session & Backend Services

## Purpose
System backend components: session manager, settings daemon, polkit agent, notification daemon, power manager, hotkey handler, GMenu proxy, clipboard manager, screen brightness helper, SDDM helper, and CPU frequency controller.

## Build
```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr && cmake --build build && sudo cmake --install build
```

## Dependencies
- Qt6 (Core, Gui, Widgets, Quick, QuickControls2, DBus, Xml, LinguistTools)
- KDE Frameworks 6 (KF6WindowSystem, KF6GlobalAccel, KF6CoreAddons, KF6IdleTime)
- PolkitQt6-1, polkit-agent-1

## Sub-components

### session (`cutefish-session`)
- Session manager that launches all desktop components
- D-Bus: `com.cutefish.Session`
- Installs session desktop files to both `/usr/share/xsessions/` and `/usr/share/wayland-sessions/`
- **Key files**: `session/application.cpp`, `session/process.cpp`, `session/processmanager.cpp`, `session/powermanager/`

### settings-daemon (`cutefish-settings-daemon`)
- Background daemon for theme, brightness, battery, language, dock, mouse, touchpad settings
- D-Bus interfaces: Brightness, Theme, PrimaryBattery, Language, Dock, Mouse, Touchpad
- **Key dirs**: `settings-daemon/theme/`, `brightness/`, `battery/`, `language/`, `dock/`, `mouse/`, `touchpad/`

### notificationd (`cutefish-notificationd`)
- Notification server with popup, window, history model
- D-Bus: `com.cutefish.Notification`, `org.freedesktop.Notifications`
- **Key files**: 33 files in `notificationd/`

### polkit-agent (`cutefish-polkit-agent`)
- PolicyKit authentication agent
- Installs binary + autostart desktop file to `/etc/xdg/autostart/`

### shutdown-ui (`cutefish-shutdown`)
- Shutdown/restart/suspend UI dialog

### powerman (`cutefish-powerman`)
- Power management: lid watcher, idle manager, CPU frequency control
- D-Bus: `com.cutefish.PowerManager`, `org.freedesktop.ScreenSaver`

### chotkeys (`chotkeys`)
- Global hotkey handler using KF6GlobalAccel

### gmenuproxy (`cutefish-gmenuproxy`)
- GTK/DBus menu proxy for global application menu
- Installs systemd user service: `cutefish-gmenuproxy.service`
- **Note**: `xembed-sni-proxy` excluded — X11/XEmbed only, not needed on Wayland

### clipboard (`cutefish-clipboard`)
- Clipboard manager

### screen-brightness (`cutefish-screen-brightness`)
- Screen brightness helper with polkit policy

### cpufreq (`cutefish-cpufreq`)
- CPU frequency controller with polkit policy

### sddm-helper (`cutefish-sddm-helper`)
- SDDM helper with polkit policy

### cupdatecursor (`cupdatecursor`)
- Cursor update utility

## Session Files
- `cutefish-wayland.desktop` → `/usr/share/wayland-sessions/` (`Exec=cutefish-session --wayland`, `DesktopNames=Cutefish`)
- `cutefish-xsession.desktop` → `/usr/share/xsessions/`
- Session files use full paths and include `X-GDM-SessionRegisters=true` for GDM session selection

## Qt5→Qt6 Migration Notes
- Qt5 → Qt6, KF5 → KF6 across all sub-components
- X11 touchpad backend excluded (Wayland uses libinput via compositor)
- Session installs Wayland session desktop file

## Status
✅ Ported, built, installed, pushed (github.com/ali-soomro)
