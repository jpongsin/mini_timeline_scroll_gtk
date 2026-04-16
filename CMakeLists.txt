cmake_minimum_required(VERSION 3.16)
project(mini_timeline_scroll)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

if(WIN32)
    set(MPV_INCLUDE_DIR "${LIBMPV_PATH}/include")
    set(MPV_LIBRARY "${LIBMPV_PATH}/libmpv.dll.a")

    add_library(mpv::mpv UNKNOWN IMPORTED)
    set_target_properties(mpv::mpv PROPERTIES
        IMPORTED_LOCATION "${MPV_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MPV_INCLUDE_DIR}"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
endif()

if(APPLE)
    add_executable(mini_timeline_scroll MACOSX_BUNDLE
        src/main.cpp
        include/video_backend.h
        include/VideoWidget.h
        src/VideoWidget.cpp
        include/MainWindow.h
        src/MainWindow.cpp
        src/video_backend.c
        src/MainWindowActions.cpp
    )
else()
    add_executable(mini_timeline_scroll
        src/main.cpp
        include/video_backend.h
        include/VideoWidget.h
        src/VideoWidget.cpp
        include/MainWindow.h
        src/MainWindow.cpp
        src/video_backend.c
        src/MainWindowActions.cpp
    )
endif()

target_compile_options(mini_timeline_scroll PRIVATE -Wall -Wextra -Wpedantic)

if(WIN32)
    set(MPV_LINKLIB mpv::mpv)
else()
    set(MPV_LINKLIB PkgConfig::MPV)
endif()

target_link_libraries(mini_timeline_scroll PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    ${MPV_LINKLIB})

if(APPLE)
    set_target_properties(mini_timeline_scroll PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.jpongsin.mini-timeline-scroll"
        MACOSX_BUNDLE_BUNDLE_NAME "mini_timeline_scroll"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE TRUE
    )
endif()

