//
// Created by jop on 3/24/26.
//

#include "video_fetch.h"

///////////////////////////////////////
///global variables
//the following variables are the exact calculation
//as specified by TV and film standards
const double fps29 = 30000.0 / 1001.0;
const double fps59 = 60000.0 / 1001.0;

double validateVideo(const char *videoFile, AVFormatContext *fic, int videoStreamIndex) {
    //else... run
    printf("Loading: %s \n", videoFile);

    //check if video is supported
    if (avformat_open_input(&fic, videoFile,NULL,NULL)) {
        printf("Could not open video file. Exiting.\n");
        exit(-1);
    }

    //check if video has stream information
    if (avformat_find_stream_info(fic,NULL) < 0) {
        avformat_close_input(&fic);
        printf("Could not find stream information. Exiting.\n");
        exit(-1);
    }

    //find all known streams from video to audio...
    for (int i = 0; i < fic->nb_streams; i++) {
        if (fic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    //if outside the index....
    if (videoStreamIndex == -1) {
        printf("Could not find video stream. Exiting.\n");
        avformat_close_input(&fic);
        exit(-1);
    }

    //fetch video info
    double new_frame = fetchVideoInfo(fic, videoStreamIndex);
    return new_frame;
}

double fetchVideoInfo(const AVFormatContext *fic, int videoStreamIndex) {
    //proceed to calculate
    AVCodecParameters *pCodecPar = fic->streams[videoStreamIndex]->codecpar;
    const char *timecode_format = "Duration: %.02ld:%.02ld:%.02ld:%.02ld seconds \n";

    //get frame rate avg
    AVStream *streams = fic->streams[videoStreamIndex];
    double frame_rate = av_q2d(streams->avg_frame_rate);
    if (frame_rate <= 0) {
        frame_rate = av_q2d(streams->r_frame_rate);
    }
    printf("Frame rate: %.3f \n", frame_rate);

    //Check if it's NDF or DF
    char *tc_mode = "Non Drop Frame";
    if (frame_rate == fps29 || frame_rate == fps59) {
        tc_mode = "Drop Frame";
        timecode_format = "Duration: %.02ld:%.02ld:%.02ld;%.02ld seconds \n";
    }
    printf("Timecode Mode: %s \n", tc_mode);

    //frame prep for timecode format.
    int frame_rounded = (int) round(frame_rate);
    //get total frames
    int64_t totalFrames = streams->nb_frames;
    //calculate FF
    int64_t framesMod = (totalFrames % frame_rounded);
    //eg. 120 seconds
    int64_t seconds_raw = (fic->duration / AV_TIME_BASE);
    //becomes 00
    int64_t seconds = seconds_raw % 60;
    //becomes 02
    int64_t minutes = seconds_raw / 60;
    //becomes 00
    int64_t hours = (minutes / 60) % 24;

    printf("Width: %d \n", pCodecPar->width);
    printf("Height: %d \n", pCodecPar->height);
    printf(timecode_format, hours, minutes, seconds, framesMod);

    return frame_rate;
}
