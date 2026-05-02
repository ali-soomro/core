find_package(KF6GlobalAccel REQUIRED)

set(PROJECT_SOURCES
    main.cpp
    application.cpp
    hotkeys.cpp
)

add_executable(chotkeys ${PROJECT_SOURCES})
target_link_libraries(chotkeys
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::DBus
    KF6::GlobalAccel
)

install(TARGETS chotkeys RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
