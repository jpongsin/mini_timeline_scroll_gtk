cmake_minimum_required(VERSION 3.16)
project(mini_timeline_scroll)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

add_executable(mini_timeline_scroll src/main.cpp
        include/video_backend.h
        include/VideoWidget.h
        src/VideoWidget.cpp
        include/MainWindow.h
        src/MainWindow.cpp
        src/video_backend.c
        src/MainWindowActions.cpp
)
target_compile_options(mini_timeline_scroll PRIVATE "-Wall" "-Wextra" "-Wpedantic")

target_link_libraries(mini_timeline_scroll PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        PkgConfig::MPV)
