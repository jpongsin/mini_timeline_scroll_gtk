// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/video_viewscaling.h"
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <string.h>

// Minimum visible pixels per axis after cropping.
// Set limitations to prevent corrupting visuals
#define ZOOM_MIN_VISIBLE 80

int zoom_is_active(const VideoPlayer *player) {
  if (!player->zcrop)
    return 0;
  gint l = 0, r = 0, t = 0, b = 0;
  g_object_get(player->zcrop,
    "left", &l,
    "right", &r,
    "top", &t,
    "bottom", &b,
               NULL);
  return (l > 0 || r > 0
    || t > 0 || b > 0);
}

void zoom_video(VideoPlayer *player, const char *action) {
  // guard: no zcrop element in pipeline
  if (!player->zcrop)
    return;

  // guard: only active on real video, not videotestsrc idle pipeline
  // uri is NULL when no file is loaded
  if (!player->uri)
    return;

  if (strcmp(action, "RESET") == 0) {
    g_object_set(player->zcrop, "left", 0, "right", 0, "top", 0, "bottom", 0,
                 NULL);
    g_print("\rZoop crop reset.");
    fflush(stdout);

    if (player->video_sink) {
        gst_video_overlay_expose(GST_VIDEO_OVERLAY(player->video_sink));
    }
    return;
  }

  // read current crop values
  gint l = 0, r = 0, t = 0, b = 0;
  g_object_get(player->zcrop, "left", &l, "right", &r, "top", &t, "bottom", &b,
               NULL);

  gboolean at_100 = (l == 0 && r == 0 && t == 0 && b == 0);

  // get actual frame dimensions from the caps on the zcrop sink pad
  // to dynamically calculate proportional steps (preserves aspect ratio)
  gint frame_w = 0, frame_h = 0;
  GstPad *pad = gst_element_get_static_pad(player->zcrop, "sink");
  if (pad) {
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (caps) {
      GstStructure *s = gst_caps_get_structure(caps, 0);
      gst_structure_get_int(s, "width", &frame_w);
      gst_structure_get_int(s, "height", &frame_h);
      gst_caps_unref(caps);
    }
    gst_object_unref(pad);
  }

  // Calculate proportional step sizes (~2% of dimensions) to prevent
  // widescreen stretching. Assign step to 20 if resolution is unknown
  gint step_x = (frame_w > 0) ? MAX(2, (gint)(frame_w * 0.02)) : 20;
  gint step_y = (frame_h > 0) ? MAX(2, (gint)(frame_h * 0.02)) : 20;

  if (strcmp(action, "ENLARGE") == 0) {
    // Check if zooming in exceed minimum visible size. Halt the enlarging.
    if (frame_w > 0 && (frame_w - l - r - 2 * step_x) < ZOOM_MIN_VISIBLE)return;
    if (frame_h > 0 && (frame_h - t - b - 2 * step_y) < ZOOM_MIN_VISIBLE)return;

    l += step_x;
    r += step_x;
    t += step_y;
    b += step_y;
  }
  else if (strcmp(action, "SHRINK") == 0) {
    //stop shrinking if 100% scale.
    if (at_100) return;

    //reduce width and height....
    int total_reduce_w = 2 * step_x;
    int total_reduce_h = 2 * step_y;

    //left and right operations
    if (l + r <= total_reduce_w) {
      l = 0;
      r = 0;
    } else {
      if (l < step_x) {
        r -= (total_reduce_w - l);
        l = 0;
      } else if (r < step_x) {
        l -= (total_reduce_w - r);
        r = 0;
      } else {
        l -= step_x;
        r -= step_x;
      }
    }

    //top and bottom operations
    if (t + b <= total_reduce_h) {
      t = 0;
      b = 0;
    } else {
      if (t < step_y) {
        b -= (total_reduce_h - t);
        t = 0;
      } else if (b < step_y) {
        t -= (total_reduce_h - b);
        b = 0;
      } else {
        t -= step_y;
        b -= step_y;
      }
    }

  } else {
    // stop WASD panning if at 100 scale
    if (at_100) return;

    // shifting crop bounds by the same amount
    //to preserve aspect ratio closely
    if (strcmp(action, "RIGHT") == 0) {
      //reaching at the end of the rightmost by
      //increasing the left and decreasing right
      int shift = MIN(r, step_x);
      l += shift;
      r -= shift;
    } else if (strcmp(action, "LEFT") == 0) {
      //going to the leftmost = 0
      //by increasing the right and decreasing the left
      int shift = MIN(l, step_x);
      l -= shift;
      r += shift;
    } else if (strcmp(action, "DOWN") == 0) {
      //same as the first two if statements
      int shift = MIN(b, step_y);
      t += shift;
      b -= shift;
    } else if (strcmp(action, "UP") == 0) {
      int shift = MIN(t, step_y);
      t -= shift;
      b += shift;
    } else {
      return; // stop if unknown action
    }
  }

  //apply changes to pipeline.
  g_object_set(player->zcrop, "left", l, "right", r, "top", t, "bottom", b,
               NULL);

  //will appear on command line
  g_print("\rZoom crop — L:%d R:%d T:%d B:%d        ", l, r, t, b);
  fflush(stdout);

  //apply to video sink.
  if (player->video_sink) {
      gst_video_overlay_expose(GST_VIDEO_OVERLAY(player->video_sink));
  }
}