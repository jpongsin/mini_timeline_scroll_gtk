//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info


#include "../include/video_hotkey_actions.h"
#include <gst/gst.h>
#include <stdio.h>
#include <gst/video/videooverlay.h>

//reverts back to the beginning of a video
void seek_begin(const VideoPlayer *player, const gint64 pos_ns) {
    if (!pipeline_is_active(player)) return;
    gst_element_seek_simple(player->pipeline,
                            GST_FORMAT_TIME,
                            GST_SEEK_FLAG_FLUSH |
                            GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_AFTER,
                            pos_ns);
}

//starts playback
gboolean start_playback(VideoPlayer *player) {
    if (!pipeline_is_active(player)) return FALSE;
    //check if the state fails
    if (gst_element_set_state(player->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline failed\n");

        //clean up from mem
        gst_element_set_state(player->pipeline, GST_STATE_NULL);
        gst_object_unref(player->pipeline);
        player->pipeline = NULL;
    }
    return G_SOURCE_REMOVE;
}

//video playback speed
void change_rate(VideoPlayer *player, gdouble direction) {
    if (!pipeline_is_active(player)) return;
    //position
    gint64 pos;

    //exit if query not found?
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &pos)) return;

    //add to the current rate from typedef struct
    player->current_rate += direction;

    // set maximum speed at 2.0 and minimum speed at 0.25
    if (player->current_rate < 0.25) player->current_rate = 0.25;
    if (player->current_rate > 2.0) player->current_rate = 2.0;

    //print on CLI
    g_print("\rNew Speed: %.2f (%s)",
            player->current_rate,
            player->current_rate == 1.0 ? "Normal" : player->current_rate > 1.0 ? "Fast" : "Slow");

    // call gst_element_seek to establish the new current rate
    // ACCURATE seek is good enough
    // TRICKMODE might be handy for 4K and up
    gst_element_seek(player->pipeline, player->current_rate,
                     GST_FORMAT_TIME,
                     GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                     GST_SEEK_TYPE_SET, pos,
                     GST_SEEK_TYPE_NONE, 0);
}

//changes the volume, changes attributes to typedef struct VideoPlayer
void change_volume(const VideoPlayer *player, gdouble direction) {
    if (!pipeline_is_active(player)) return;
    //get current volume
    gdouble current_vol;
    g_object_get(player->volume_stream, "volume", &current_vol, NULL);

    //add or decrease volume
    current_vol += direction;

    //set maximum volume at 200, minimum volume at 0
    if (current_vol < 0.0) current_vol = 0.0;
    if (current_vol > 2.0) current_vol = 2.0;

    //set the new volume
    g_object_set(player->volume_stream, "volume", current_vol, NULL);

    //print volume
    g_print("\rVolume: %.0f%%", current_vol * 100);
}

// Helper for precise H:M:S:F display (used by both)
void print_seek_label(VideoPlayer *player, gint64 target_ns, gint64 jump) {
    double fps = player->assignedFPS;

    // obtain seconds, hours, minutes...
    int total_seconds = (int)(target_ns / GST_SECOND);
    int h = total_seconds / 3600;
    int m = (total_seconds / 60) % 60;
    int s = total_seconds % 60;

    // get frame index
    gint64 frame_idx = gst_util_uint64_scale(target_ns, (int)(fps * 1000 + 0.5), 1000 * GST_SECOND);

    // check if 29 or 59; range
    gboolean is_drop = (fps > 29.9 && fps < 30.0) || (fps > 59.9 && fps < 60.0);
    int f_label;

    if (is_drop) {
        int drop_val = (fps > 40) ? 4 : 2;
        int chunk = (fps > 40) ? 35964 : 17982;
        gint64 D = frame_idx / chunk;
        gint64 M = frame_idx % chunk;
        // adjust frame rates for display
        gint64 adjusted = frame_idx + (18 * (drop_val/2) * D) + drop_val * ((M - drop_val) / 1798);
        f_label = (int)(adjusted % (int)(fps + 0.5));
    } else {
        f_label = (int)(frame_idx % (int)(fps + 0.5));
    }

    g_print("\r Frame Jump: %ld | Duration: %02d:%02d:%02d%c%02d",
            (long)jump, h, m, s, is_drop ? ';' : ':', f_label);
    fflush(stdout);
}

void seek_mechanism(VideoPlayer *player, gint64 delta) {
    //if pipeline idling, protect from errors
    if (!pipeline_is_active(player)) return;

    // assign current position
    gint64 current_pos;
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &current_pos)) return;

    // obtain frame rates from sink for accuracy
    int num = 0, den = 0;
    GstPad *pad = gst_element_get_static_pad(player->video_sink, "sink");
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (caps) {
        GstStructure *s = gst_caps_get_structure(caps, 0);
        gst_structure_get_fraction(s, "framerate", &num, &den);
        gst_caps_unref(caps);
    }
    gst_object_unref(pad);

    // handle fallback if caps not ready
    if (num <= 0 || den <= 0) {
        num = (int)(player->assignedFPS * 1000 + 0.5);
        den = 1000;
    }

    //calculate current and target frames
    gint64 current_f = gst_util_uint64_scale(current_pos, num, den * GST_SECOND);
    gint64 target_f = current_f + delta;
    if (target_f < 0) target_f = 0;

    // find the middle of target frame
    gint64 frame_duration = gst_util_uint64_scale(den, GST_SECOND, num);
    gint64 target_ns = (target_f * frame_duration) + (frame_duration / 2);

    //checking on CLI
    print_seek_label(player, target_ns, delta);

    //perform an element seek and make it look like seeking
    gst_element_seek(player->pipeline, 1.0, GST_FORMAT_TIME,
                     (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                     GST_SEEK_TYPE_SET, target_ns,
                     GST_SEEK_TYPE_NONE, -1);
}


// derivative of seek_mechanism, for use of seeking by one frame
void on_key_right(VideoPlayer *p) {
    seek_mechanism(p, 1);
}
void on_key_left(VideoPlayer *p) {
    seek_mechanism(p, -1);
}

// check if pipeline is active. check if state is playing, then pause, else: play.
void toggle_playback(const VideoPlayer *player) {
    if (!pipeline_is_active(player)) return;
    GstState current, pending;
    gst_element_get_state(player->pipeline, &current, &pending, 0);
    if (current == GST_STATE_PLAYING || pending == GST_STATE_PLAYING) {
        gst_element_set_state(player->pipeline, GST_STATE_PAUSED);
        gst_video_overlay_expose(GST_VIDEO_OVERLAY(player->video_sink));
        g_print("\rPaused");
    } else {
        gst_element_set_state(player->pipeline, GST_STATE_PLAYING);
        g_print("\rPlaying");
    }
    fflush(stdout);
}

void toggle_mute(const VideoPlayer *player) {
    if (!pipeline_is_active(player)) return;
    gboolean muted;
    g_object_get(player->volume_stream, "mute", &muted, NULL);

    gboolean next_state = !muted;
    g_object_set(player->volume_stream, "mute", next_state, NULL);

    // Using \r and spaces to overwrite the previous line
    g_print("\rStatus: %s          ", next_state ? "Muted  " : "Unmuted");
    fflush(stdout);
}
// returns TRUE if pipeline is ready for hotkey actions
gboolean pipeline_is_active(const VideoPlayer *player) {
    return player->pipeline != NULL
        && player->assignedFPS > 0.0
        && player->volume_stream != NULL;
}