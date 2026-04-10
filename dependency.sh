#!/bin/sh
set -e

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "OS not found"
    exit 1
fi

echo "Your OS is: $OS"

case "$OS" in
    ubuntu|debian)
        echo "Installing for Ubuntu/Debian..."
        sudo apt update
        # FFmpeg & Build Essentials
        sudo apt install -y build-essential cmake pkg-config \
            libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
        # GStreamer (Base + App + Video)
        sudo apt install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
        # Qt6
        sudo apt install -y qt6-base-dev libqt6widgets6
        ;;

    opensuse-tumbleweed|opensuse-leap|opensuse)
        echo "Installing for openSUSE..."
        sudo zypper refresh
        # Common build tools
        sudo zypper install -y -t pattern devel_basis
        sudo zypper install -y cmake pkg-config
        # FFmpeg Development Libraries
        sudo zypper install -y ffmpeg-7-libavcodec-devel ffmpeg-7-libavformat-devel \
            ffmpeg-7-libavutil-devel ffmpeg-7-libswscale-devel
        # GStreamer (contains app-1.0 and video-1.0)
        sudo zypper install -y gstreamer-devel gstreamer-plugins-base-devel
        # Qt6
        sudo zypper install -y qt6-base-devel qt6-widgets-devel
        ;;
    arch)
        echo "Installing for Arch Linux..."
        # base-devel = make essential
        sudo pacman -Syu --needed base-devel cmake
        # FFmpeg & GStreamer
        sudo pacman -S --needed ffmpeg gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad
        # Qt6
        sudo pacman -S --needed qt6-base
        ;;

    fedora|rhel|centos)
        echo "Installing for Fedora/RedHat (dnf)..."
        # Development tools group
        sudo dnf group install -y c-development development-tools
        sudo dnf install -y cmake pkgconf-pkg-config
        # install FFmpeg and libav
        sudo dnf install -y ffmpeg-devel libavcodec-free-devel libavformat-free-devel libswscale-free-devel
        # GStreamer
        sudo dnf install -y gstreamer1-devel gstreamer1-plugins-base-devel
        # Qt6 Dev
        sudo dnf install -y qt6-qtbase-devel qt6-qtdeclarative-devel
        ;;
    freebsd)
        echo "Installing for FreeBSD..."
        # make sure sudo is installed
        sudo pkg install -y cmake pkgconf \
            ffmpeg gstreamer1 gstreamer1-plugins gstreamer1-libav vulkan-headers \
            qt6-base qt6-declarative
        ;;
    macos)
        echo "Installing for macOS via Homebrew..."
        if ! command -v brew &> /dev/null; then
            echo "Homebrew not found. Please install it from https://brew.sh/"
            exit 1
        fi
        # install everything at once
        brew install cmake pkg-config ffmpeg gstreamer gst-plugins-base qt6
        ;;
    *)
        echo "OS '$OS' is not yet supported by this script."
        exit 1
        ;;
esac

echo "You may now run ./make_timeline_scroll.sh"


