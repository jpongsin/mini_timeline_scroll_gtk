//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info


#include "../include/video_fetch.h"

///////////////////////////////////////
///global variables
//the following variables are the exact calculation
//as specified by TV and film standards
const double fps29 = 30000.0 / 1001.0;
const double fps59 = 60000.0 / 1001.0;

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
    //Check if it's NDF or DF
    char *tc_mode = "Non Drop Frame";
    if (frame_rate == fps29 || frame_rate == fps59) {
        tc_mode = "Drop Frame";
        timecode_format = "Duration: %.02ld:%.02ld:%.02ld;%.02ld seconds \n";
    }
    //frame prep for timecode format.
    int64_t total_seconds = fic->duration / AV_TIME_BASE;
    int64_t seconds = total_seconds % 60;
    int64_t total_minutes = total_seconds / 60;
    int64_t minutes = total_minutes % 60;
    int64_t hours = total_minutes / 60;

    //ff calculate for frame
    int frame_rounded = (int)round(frame_rate);
    int64_t totalFrames = streams->nb_frames;
    int64_t framesMod = (totalFrames % frame_rounded);

    // CLI info
    printf("Frame rate: %.3f\n", frame_rate);
    printf("Timecode Mode: %s\n", tc_mode);
    printf("Width: %d | Height: %d\n", pCodecPar->width, pCodecPar->height);
    printf(timecode_format, hours, minutes, seconds, framesMod);

    return frame_rate;
}

VideoMediaInfo load_video_for_qt(const char *videoFile) {
    VideoMediaInfo info = {0};
    AVFormatContext *fic = NULL;

    printf("Loading via QT interface: %s \n", videoFile);
    if (avformat_open_input(&fic, videoFile, NULL, NULL)) {
        printf("Could not open video file. Exiting.\n");
        return info;
    }

    if (avformat_find_stream_info(fic, NULL) < 0) {
        avformat_close_input(&fic);
        printf("Could not find stream information. Exiting.\n");
        return info;
    }

    // Process video stream for timecode calculation and FPS
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < fic->nb_streams; i++) {
        if (fic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = (int)i;
            break;
        }
    }

    if (videoStreamIndex != -1) {
        info.fps = fetchVideoInfo(fic, videoStreamIndex);
    }

    // Process audio streams
    int audio_count = 0;
    for (unsigned int i = 0; i < fic->nb_streams; i++) {
        if (fic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_count++;
        }
    }

    info.audio_count = audio_count;
    if (audio_count > 0) {
        info.audio_tracks = malloc(sizeof(AudioTrackInfo) * audio_count);

        int track_idx = 0;
        int track_num = 1;
        for (unsigned int i = 0; i < fic->nb_streams; i++) {
            if (fic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                AVDictionaryEntry *lang = av_dict_get(fic->streams[i]->metadata, "language", NULL, 0);
                AVDictionaryEntry *title = av_dict_get(fic->streams[i]->metadata, "title", NULL, 0);

                char buffer[512] = {0};
                if (title && strlen(title->value) > 0) {
                    snprintf(buffer, sizeof(buffer), "%s", title->value);
                } else {
                    snprintf(buffer, sizeof(buffer), "Track %d", track_num);
                }

                if (lang && strlen(lang->value) > 0) {
                    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " (%s)", lang->value);
                }

                info.audio_tracks[track_idx].index = track_idx; // UI combobox data
                info.audio_tracks[track_idx].label = strdup(buffer);
                
                track_idx++;
                track_num++;
            }
        }
    }

    avformat_close_input(&fic);
    return info;
}

void free_video_media_info(VideoMediaInfo *info) {
    //audio track exists?
    if (info->audio_tracks) {
        //free memory of audio tracks once used up.
        for (int i = 0; i < info->audio_count; i++) {
            if (info->audio_tracks[i].label) {
                free(info->audio_tracks[i].label);
            }
        }
        free(info->audio_tracks);
        info->audio_tracks = NULL;
    }
    info->audio_count = 0;
}
