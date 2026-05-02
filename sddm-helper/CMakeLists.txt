project(cutefish-sddm-helper)

add_executable(cutefish-sddm-helper main.cpp)
target_link_libraries(cutefish-sddm-helper Qt6::Core Qt6::Gui)

configure_file(
    com.cutefish.sddm.helper.pkexec.policy.in
    com.cutefish.sddm.helper.pkexec.policy
    @ONLY
)

install(TARGETS cutefish-sddm-helper RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.cutefish.sddm.helper.pkexec.policy
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/polkit-1/actions/)
