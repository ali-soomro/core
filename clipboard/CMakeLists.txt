cmake_minimum_required(VERSION 3.16)
project(cutefish-clipboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cutefish-clipboard main.cpp clipboard.cpp)
target_link_libraries(cutefish-clipboard Qt6::Core Qt6::Gui Qt6::Widgets)
install(TARGETS cutefish-clipboard DESTINATION ${CMAKE_INSTALL_BINDIR})
