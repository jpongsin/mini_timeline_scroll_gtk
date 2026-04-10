## mini_timeline_scroll
![A window opening, with a menu bar, file information and time code sandwiched between a video stream.](https://github.com/jpongsin/mini_timeline_scroll/blob/main/samples/mini_timeline_scroll_1.png)

#### Description:
This program takes a video file
as an argument. When you pick the video file, the program
will check if there is any information
on the video.
It then opens a screen.
This program is written for Linux, FreeBSD, and MacOS.

The timecode and hotkey labels are shown on the window,
with the timecode on the bottom of screen
and the hotkeys on the top of screen.

Unfortunately, it does not have a visual scroller.
You will have to rely on the following key shortcuts: 

* j/shift+left arrow (<-). step backward 10 frame 
* k and spacebar. play and pause 
* l/shift+right arrow (->). step forward 10 frame 
* left arrow (<-). step -1 frame 
* right arrow (->). step +1 frame 
* left bracket. decrease speed (-0.25) 
* right bracket. increase speed (+0.25) 
* down arrow (^). decrease volume -0.10 level 
* up arrow (v). increase volume +0.10 level 
* home key (HOME). rewind to beginning
* f. fullscreen 
* m. mute/unmute

Fortunately, you can exit the video window, meaning
the application will exit and terminate.

#### Requirements:
* ffmpeg >= 6
* gstreamer >= 1.0
* Qt >= 6.10 
* Homebrew (MacOS only)
* CMake >= 4.2

#### Instructions: 
1. Run ./dependency.sh to install dependencies. 
2. Run ./make_timeline_scroll.sh. 
3. Go to /build. You will find an executable app "mini_timeline_scroll"
4. Click "mini_timeline_scroll". Make sure to tick "Executable as Program" in Properties.
5. You may also type "./timeline_scroll_Qt video_name.mov" to open video instantly.

#### Notes:
* Prototype. It is recommmended to do a fresh build as versions of ffmpeg and its libraries are not backwards compatible.
* No support for Wayland as of April 2026. There will be ways to address this
* If you are using a laptop with mixed graphics (e.g. Intel + NVIDIA), if you want to get the most performance, run "Performance Mode", or prime-run, or add an offload prefix.
* Maximum streamable file reportedly is 8K 30fps.
* May not cooperate with high bitrates at all.
* Limited compatibility with HDR and some select proprietary formats

Copyright (C) 2026 jpongsin
