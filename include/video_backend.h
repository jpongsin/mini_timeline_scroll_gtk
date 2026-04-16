// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include <mpv/client.h>
#include <mpv/render_gl.h>

typedef struct {
  int index;
  char language[16];
  char title[128];
} AudioTrack;

typedef struct {
  double duration;
  double fps;
  int width;
  int height;
  char filename[512];
  char codec_name[64];
  int hw_accel_used;

  AudioTrack audio_tracks[16];
  int nb_audio_tracks;
  int current_audio_track;
} VideoState;

#ifdef __cplusplus
extern "C" {
#endif

// Returns a handle to the backend (mpv_handle)
mpv_handle *init_backend(void);
void configure_mpv_handle(mpv_handle *handle);

// Grabs current metadata from mpv properties
VideoState get_metadata(mpv_handle *handle);
void get_video_dimensions(VideoState *vs, mpv_node node);
void get_video_metadata(mpv_node track, char **type, int *id, char **lang, char **title_str);
void get_audio_metadata(VideoState *vs, char *type, int id, char *lang, char *title_str);
void get_stream_metadata(VideoState *vs, mpv_node track);
void get_overall_metadata(VideoState *vs, mpv_node node);

// Core playback controls
void load_file(mpv_handle *handle, const char *filename);
void toggle_pause(mpv_handle *handle);
void seek_absolute(mpv_handle *handle, double seconds, int exact);
void set_audio_track(mpv_handle *handle, int track_id);
void set_zoom(mpv_handle *handle, double zoom);
void set_pan(mpv_handle *handle, double x, double y);

void cleanup_backend(mpv_handle *handle);

#ifdef __cplusplus
}
#endif
