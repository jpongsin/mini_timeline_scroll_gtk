## mini_timeline_scroll

#### Description:
This program takes a video file
as an argument. When you pick the video file, the program
will check if there is any information
on the video.
It then opens a screen.
This program is written for Linux and MacOS.

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
* ffmpeg
* gstreamer
* Qt6
* Homebrew

#### Instructions:
If running from the executable
1. Open timeline_scroll_Qt
2. Open the video

If running from command line
1. Type make on the terminal
2. Type "./timeline_scroll_Qt video_name.mov"
3. To clean up, run make clean.


#### Expected Output:
![A window opening, with a menu bar, file information and time code sandwiched between a video stream.](https://github.com/jpongsin/mini_timeline_scroll/blob/main/samples/mini_timeline_scroll_1.png)

#### Notes:
* Prototype.
* Maximum streamable file reportedly is 8K 30fps.
* May not cooperate with high bitrates at all.
* Limited compatibility with HDR and some select proprietary formats

Copyright (C) 2026 jpongsin
