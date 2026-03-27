//
// Created by jop on 3/24/26.
//

#ifndef CLI_TIMELINE_SCRUB_VIDEO_FETCH_H
#define CLI_TIMELINE_SCRUB_VIDEO_FETCH_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <stdio.h>
#include <math.h>

#include "video_implement.h"


///////////////////////////////////////
///global variables
//using extern to prototype.... defined in video_fetch.c
extern const double fps29;
extern const double fps59;

////////////////////////////////////////
///prototypes
double fetchVideoInfo(const AVFormatContext *fic, int videoStreamIndex);

double validateVideo(const char *videoFile, AVFormatContext *fic, int videoStreamIndex);

///////////////////////////////////////
#endif //CLI_TIMELINE_SCRUB_VIDEO_FETCH_H
