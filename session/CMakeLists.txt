project(cutefish-session)
set(TARGET cutefish-session)

set(SOURCES
    application.cpp
    main.cpp
    process.cpp
    processmanager.cpp
    networkproxymanager.cpp
    powermanager/power.cpp
    powermanager/powerproviders.cpp
)

qt6_add_dbus_adaptor(DBUS_SOURCES
                     com.cutefish.Session.xml
                     application.h Application
                     sessionadaptor SessionAdaptor)
set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

add_executable(${TARGET} ${SOURCES} ${DBUS_SOURCES})
target_link_libraries(${TARGET}
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::DBus
        KF6::WindowSystem
)

install(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install both X11 and Wayland session entries
install(FILES cutefish-xsession.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/xsessions/)
install(FILES cutefish-wayland.desktop  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wayland-sessions/)
