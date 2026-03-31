//
// Created by jop on 3/25/26.
//

#ifndef CLI_TIMELINE_SCRUB_TIMECODE_CHECK_H
#define CLI_TIMELINE_SCRUB_TIMECODE_CHECK_H
#include <stdio.h>
#include <math.h>

#include <gdk/gdk.h>
#include <gst/gst.h>
#include <gtk/gtk.h>

#include "video_implement.h"

gboolean on_ui_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer data);
void update_live_timecode(VideoPlayer *player);

#endif //CLI_TIMELINE_SCRUB_TIMECODE_CHECK_H
