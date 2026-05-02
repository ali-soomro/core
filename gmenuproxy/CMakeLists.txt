find_package(AppMenuGtkModule)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6CoreAddons REQUIRED)
set_package_properties(AppMenuGtkModule PROPERTIES TYPE RUNTIME)

add_definitions(-DQT_NO_CAST_TO_ASCII
-DQT_NO_CAST_FROM_ASCII
-DQT_NO_CAST_FROM_BYTEARRAY)

set(GMENU_DBUSMENU_PROXY_SRCS
    extend/dbusmenutypes_p.cpp
    main.cpp
    menuproxy.cpp
    window.cpp
    menu.cpp
    actions.cpp
    dbusmenuadaptor.cpp
    gdbusmenutypes_p.cpp
    icons.cpp
    utils.cpp
)

add_executable(cutefish-gmenuproxy ${GMENU_DBUSMENU_PROXY_SRCS})
target_link_libraries(cutefish-gmenuproxy
    Qt6::Core
    Qt6::DBus
    Qt6::Widgets
    KF6::CoreAddons
    KF6::WindowSystem
    xcb
)

configure_file(
    cutefish-gmenuproxy.service.in
    cutefish-gmenuproxy.service
    @ONLY
)

install(TARGETS cutefish-gmenuproxy DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cutefish-gmenuproxy.service
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user/)
