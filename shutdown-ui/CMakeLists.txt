project(cutefish-shutdown)

add_executable(cutefish-shutdown
    main.cpp
    actions.cpp
    qml.qrc
)

target_link_libraries(cutefish-shutdown
        Qt6::Core
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::DBus
)

file(GLOB TS_FILES translations/*.ts)
qt6_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
add_custom_target(shutdown-translations DEPENDS ${QM_FILES} SOURCES ${TS_FILES})
add_dependencies(cutefish-shutdown shutdown-translations)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cutefish-shutdown/translations)
install(TARGETS cutefish-shutdown RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
