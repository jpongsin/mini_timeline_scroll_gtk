#!/bin/sh

build_folder="build"

if [ -d "$build_folder" ]; then
	rm -rf "$build_folder"
fi

#ensure you are in main folder before building
mkdir "$build_folder"
cd "$build_folder"

# Search for the Qt6 configuration file in common areas
QT_PATH=$(find $HOME /opt /usr /usr/local -name "Qt6Config.cmake" -print -quit 2>/dev/null | sed 's|/lib/cmake/Qt6/Qt6Config.cmake||')

if [ -n "$QT_PATH" ]; then
    echo "Found Qt6 at: $QT_PATH"
    cmake -DCMAKE_PREFIX_PATH="$QT_PATH" ..
else
    echo "Qt6 not found. Please install it or set CMAKE_PREFIX_PATH"
    exit 1
fi
 
cmake --build . --parallel

#to make clean you run
#cmake --build . --target clean
