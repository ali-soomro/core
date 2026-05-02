project(cutefish-screen-brightness)
set(PROJECT_NAME cutefish-screen-brightness)

add_executable(${PROJECT_NAME}
    main.cpp
    brightnesshelper.h
    brightnesshelper.cpp
)

target_link_libraries(${PROJECT_NAME}
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

configure_file(
    com.cutefish.brightness.pkexec.policy.in
    com.cutefish.brightness.pkexec.policy
    @ONLY
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.cutefish.brightness.pkexec.policy
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/polkit-1/actions/)
