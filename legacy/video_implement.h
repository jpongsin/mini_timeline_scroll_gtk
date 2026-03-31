//
// Created by jop on 3/24/26.
//

#ifndef CLI_TIMELINE_SCRUB_VIDEO_IMPLEMENT_H
#define CLI_TIMELINE_SCRUB_VIDEO_IMPLEMENT_H

#include <gdk/gdk.h>
#include <gst/gst.h>
#include <gtk/gtk.h>

typedef struct {
    GstElement *pipeline;
    GstElement *gtk_sink;

    //pairs of streams to be queued
    GstElement *video_entry;
    GstElement *audio_entry;

    //volume control
    GstElement *volume_stream;

    //check speed
    gdouble current_rate;

    //booleans that speak for itself
    gboolean is_playing;
    gboolean is_muted;
    gboolean is_seeking;
    gboolean contains_audio;

    //for use in calculation
    gdouble assignedFPS;
    gint64 duration;

    //display acceleration
    const gchar* accel_type;
    GtkWidget *timecode_show;

    // for file name parsing
    char *uri;
} VideoPlayer;

gboolean start_playback(VideoPlayer *player);

void init_video_processor(VideoPlayer *player, const char *path);

void seek_frames(VideoPlayer *player, gint64 direction);

void seek_begin(VideoPlayer *player, gint64 pos_ns);

void change_rate(VideoPlayer *player, gdouble new_rate);

void change_volume(VideoPlayer *player, gdouble direction);

void on_pad_added(GstElement *src, GstPad *new_pad, gpointer data);
#endif //CLI_TIMELINE_SCRUB_VIDEO_IMPLEMENT_H
