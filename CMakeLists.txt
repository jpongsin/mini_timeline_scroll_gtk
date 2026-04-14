cmake_minimum_required(VERSION 3.16)
project(mini_timeline_scroll)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    if (EXISTS "/opt/homebrew")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
        set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
    set(EXE_TYPE MACOSX_BUNDLE)
else()
    find_package(X11 REQUIRED)
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswscale
)

pkg_check_modules(GST REQUIRED IMPORTED_TARGET
        gstreamer-1.0
        gstreamer-video-1.0
        gstreamer-base-1.0
        gstreamer-app-1.0
)

find_program(NVIDIA_SMI "nvidia-smi")

if(NVIDIA_SMI)
    set(NVIDIA_FOUND TRUE)
    add_definitions(-D__NVIDIA__)
    message(STATUS "NVIDIA hardware detected on this system")
endif()

add_executable(mini_timeline_scroll ${EXE_TYPE}
        src/main.cpp
        include/timecode_check.h
        include/video_fetch.h
        src/video_implement.c
        src/video_fetch.c
        src/timecode_check.c
        src/video_window.cpp
        include/video_window.h
        src/video_hotkey_actions.c
        include/video_hotkey_actions.h
        include/video_export_actions.h
        src/video_export_actions.c
        src/video_windowloads.cpp
        include/video_viewscaling.h
        src/video_viewscaling.c
)
if(NVIDIA_FOUND)
    target_compile_definitions(mini_timeline_scroll PRIVATE __NVIDIA__)
endif()
target_link_libraries(mini_timeline_scroll PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        PkgConfig::FFMPEG
        PkgConfig::GST)

if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE objc "-framework Cocoa")

    set_target_properties(mini_timeline_scroll PROPERTIES
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.jpongsin.timeline-scrub"
            MACOSX_BUNDLE_BUNDLE_NAME "Mini Timeline View"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
            INSTALL_RPATH "/opt/homebrew/lib"
            BUILD_WITH_INSTALL_RPATH TRUE
    )
else()
    target_link_libraries(mini_timeline_scroll PRIVATE
            ${X11_LIBRARIES})
endif()
