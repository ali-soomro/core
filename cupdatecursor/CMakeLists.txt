project(cupdatecursor)

add_executable(cupdatecursor main.cpp)
target_link_libraries(cupdatecursor
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
)

install(TARGETS cupdatecursor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
