## mini_timeline_scroll

#### Description:
This program takes a video file
as an argument. When you pick the video file, the program
will check if there is any information
on the video.
It then opens a screen.

The timecode and hotkey labels are shown on the window,
with the timecode on the bottom left
and the hotkeys on the bottom right.

Unfortunately, it does not have a visual scroller.
You will have to rely on the following key shortcuts: 

j/shift+left arrow (<-). step backward 10 frame \
k and spacebar. play and pause \
l/shift+right arrow (->). step forward 10 frame\

left arrow (<-). step -1 frame \
right arrow (->). step +1 frame \

left bracket. decrease speed (-0.25)\
right bracket. increase speed (+0.25)\

down arrow (^). decrease -0.10 level \
up arrow (v). increase +0.10 level \

home key (HOME). rewind to beginning\
f. fullscreen \
m. mute/unmute

Fortunately, you can exit the video window, meaning
the application will exit and terminate.

#### Requirements:
* ffmpeg
* gtk4
* gstreamer

#### Instructions:
1. Type make on the terminal
2. Type "./mini_timeline_scroll video_name.mov"
3. To clean up, run make clean.


Expected Output:
A window opening, with instructions and time code sandwiched between a video stream.

Notes:
Prototype.
This video runs on software acceleration. Maximum streamable file reportedly is 8K 30fps.
May need to look for options for hardware and GUI

Copyright (C) 2026 jpongsin
