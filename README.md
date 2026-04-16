## mini_timeline_scroll
![A window opening, with a menu bar, file information and time code sandwiched between a video stream.](https://github.com/jpongsin/mini_timeline_scroll/blob/main/samples/mini_timeline_scroll_1.png)

#### Description:
This program takes a video file
as an argument. When you pick the video file, the program
will check if there is any information
on the video.
It then opens a screen.
This program is written for Linux (>=6.8), FreeBSD (>=15), and MacOS(>=15).

The timecode and hotkey labels are shown on the window,
with the timecode on the bottom of screen
and the hotkeys on the top of screen.

#### Requirements:
* CMake >= 3
* Qt >= 6.10
* libmpv
* ffmpeg >= 6
* Homebrew (MacOS only)


#### Instructions: 
1. Run ./dependency.sh to install dependencies. 
2. Run ./make_timeline_scroll.sh. 
3. Go to /build. You will find an executable app "mini_timeline_scroll"
4. Click "mini_timeline_scroll". Make sure to tick "Executable as Program" in Properties.
5. You may also type "./mini_timeline_scroll video_name.mov" to open video instantly.

#### How to Operate:
This program is designed primarily for keyboards.
You will have to rely on the following key shortcuts: 

to use playback:
* j/shift+left arrow (<-). step backward 5 seconds
* k and spacebar. play and pause 
* l/shift+right arrow (->). step forward 5 seconds
* left arrow (<-). step -1 frame 
* right arrow (->). step +1 frame
* home key (HOME). rewind to beginning

to change playback speed:
* left bracket. decrease speed (-0.25) 
* right bracket. increase speed (+0.25)

to change volume:
* down arrow (^). decrease volume -0.10 level 
* up arrow (v). increase volume +0.10 level
* m. mute/unmute

to zoom a video:
* i, reset zoom
* o, zoom out
* p, zoom in
* w, scroll to top while zooming
* s, scroll to down ...
* a, scroll to left ...
* d, scroll to right ...

other features:
* b, screenshot a frame
* f. fullscreen
* f11, autohide toolbar

Fortunately, you can exit the video window, meaning the application will exit and terminate.\
You can also use the mouse to navigate the file menu. There will be clickable features at some point.

#### Notes:
* Prototype. It is recommmended to do a fresh build as versions of ffmpeg and its libraries are not backwards compatible.
* X11 and Wayland tested
* For laptop with mixed graphics (e.g. Intel + NVIDIA): If you want to get the most performance, run "Performance Mode", or prime-run, or add an offload prefix.
* Maximum streamable file reportedly is 8K 30fps.

Copyright (C) 2026 jpongsin
