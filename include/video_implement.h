// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#ifndef TIMELINE_SCRUB_QT_VIDEO_IMPLEMENT_H
#define TIMELINE_SCRUB_QT_VIDEO_IMPLEMENT_H

#include <gst/gst.h>

#ifdef __cplusplus
extern "C" {

#endif
typedef struct {
  // qt window assignment
  // declare pipeline
  // declare video sink
  GstElement *pipeline;
  GstElement *video_sink;
  unsigned long window_id;

  // declare video and audio queues and volume knob
  GstElement *video_entry;
  GstElement *audio_entry;
  GstElement *volume_stream;
  // zoom/pan crop element — inserted between videoconvert and sink chain
  // NULL when no pipeline is active or on idle (videotestsrc) pipeline
  GstElement *zcrop;

  // declare current speed,
  // check if playing, muted, seeking, or contains audio
  gdouble current_rate;
  gboolean is_playing;
  gboolean is_muted;
  gboolean is_seeking;
  gboolean contains_audio;

  // check if an FPS has assigned
  // obtain duration
  // obtain acceleration type for hardware/software
  // obtain uri for file opening
  gdouble assignedFPS;
  gint64 duration;
  const gchar *accel_type;
  const char *uri;

  // for use in init_video_processor
  // selects audio streams;eg english, french
  gint use_hw_accel;
  gint selected_audio_stream;
  gint audio_pad_count;
} VideoPlayer;

// for detecting gpu
typedef enum { SINK_GL, SINK_OSX, SINK_XV, SINK_SOFTWARE } VideoSinkType;

// prototypes
void set_video_window(VideoPlayer *player, unsigned long window_handle);

void init_video_processor(VideoPlayer *player, const char *path);

void on_pad_added(GstElement *src, GstPad *new_pad, gpointer data);

void init_idle_pipeline(VideoPlayer *player);
#ifdef __cplusplus
}
#endif

#endif // TIMELINE_SCRUB_QT_VIDEO_IMPLEMENT_H
