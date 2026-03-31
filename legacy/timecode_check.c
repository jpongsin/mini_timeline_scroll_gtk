//
// Created by jop on 3/25/26.
//
#include "timecode_check.h"

void update_live_timecode(VideoPlayer *player) {
    gint64 pos_ns;
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &pos_ns)) return;

    // get total frame
    double fps = player->assignedFPS;
    gint64 frame_duration = (gint64)(GST_SECOND / fps);
    gint64 total_frames = (pos_ns + (frame_duration / 2)) / frame_duration;

    int hours, minutes, seconds, frames;
    gboolean is_drop = FALSE;
    char separator = ':';

    //drop frame check
    if (fps > 29.9 && fps < 30.0) {
        is_drop = TRUE;
        separator = ';';
    } else if (fps > 59.9 && fps < 60.0) {
        is_drop = TRUE;
        separator = ';';
    }

    //set up drop frame calculation for the label
    if (is_drop) {
        int drop_per_min = (fps > 50) ? 4 : 2;
        gint64 frames_per_10_min = (fps > 50) ? 35964 : 17982;

        gint64 d = total_frames / frames_per_10_min;
        gint64 m = total_frames % frames_per_10_min;

        // per minute jump
        if (m >= drop_per_min) {
            total_frames += (9 * drop_per_min * d) +
                            (drop_per_min * ((m - drop_per_min) / 1798));
        }
        //every 10 minute, no drops
        else {
            total_frames += (9 * drop_per_min * d);
        }
    }

    //turn frame into timecode
    int fps_int = (int) (fps + 0.5);
    frames = (int) (total_frames % fps_int);
    seconds = (int) ((total_frames / fps_int) % 60);
    minutes = (int) ((total_frames / (fps_int * 60)) % 60);
    hours = (int) ((total_frames / (fps_int * 3600)));

    gchar *tc_text = g_strdup_printf("%02d:%02d:%02d%c%02d",
                                     hours, minutes, seconds, separator, frames);

    gtk_label_set_text(GTK_LABEL(player->timecode_show), tc_text);
    g_free(tc_text);
}


//check if the video is continuing in the pipeline
//then update
gboolean on_ui_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer data) {
    VideoPlayer *player = (VideoPlayer *) data;
    if (player->pipeline) {
        update_live_timecode(player);
    }
    return G_SOURCE_CONTINUE;
}
