//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info

#ifndef TIMELINE_SCROLL_VIDEO_VIEWSCALING_H
#define TIMELINE_SCROLL_VIDEO_VIEWSCALING_H

#include "video_implement.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Zoom and pan via videocrop element.
// if scale is at 100, block WASD and shrink mechanism
// only active on an active video pipeline
// it updates the videocrop element from video_implement.c
void zoom_video(VideoPlayer *player, const char *action);

// if zoomed in, return 1; else return 0 if 100% scale
int zoom_is_active(const VideoPlayer *player);

#ifdef __cplusplus
}
#endif

#endif //TIMELINE_SCROLL_VIDEO_VIEWSCALING_H