#include "video_implement.h"

void on_pad_added(GstElement *src, GstPad *new_pad, gpointer data) {
    //define player, sinkpad
    VideoPlayer *player = (VideoPlayer *) data;
    GstPad *sink_pad = NULL;

    //tell to use threads
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(src), "max-threads")) {
        g_object_set(src, "max-threads", 0, NULL);
    }

    //query the current caps and then get it
    GstCaps *caps = gst_pad_get_current_caps(new_pad);
    GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(str);

    //names
    //if structure matches name, add to video entry and audio entry
    if (g_str_has_prefix(name, "video/x-raw")) {
        sink_pad = gst_element_get_static_pad(player->video_entry, "sink");
    } else if (g_str_has_prefix(name, "audio/x-raw")) {
        sink_pad = gst_element_get_static_pad(player->audio_entry, "sink");
    }
    //if not linked already
    if (sink_pad && !gst_pad_is_linked(sink_pad)) {
        gst_pad_link(new_pad, sink_pad);
    }

    //unref
    if (sink_pad) gst_object_unref(sink_pad);
    if (caps) gst_caps_unref(caps);
}

void init_video_processor(VideoPlayer *player, const char *path) {
    //define the current rate; so that the speed meter is properly initialized
    player->current_rate = 1.0;

    //make a new pipeline
    player->pipeline = gst_pipeline_new("universal-player");

    // a file source to process the open file, then decode the file
    GstElement *src = gst_element_factory_make("filesrc", "src");
    GstElement *dbin = gst_element_factory_make("decodebin", "dbin");

    //then in video , a queue is made to account for high bitrate video
    //then converting, scaling, limiting the scale of video
    //and paintable sink for the gui
    player->video_entry = gst_element_factory_make("queue", "v_queue");
    GstElement *v_conv = gst_element_factory_make("videoconvert", "v_conv");
    GstElement *v_scale = gst_element_factory_make("videoscale", "v_scale");
    GstElement *v_limit = gst_element_factory_make("capsfilter", "v_limit");
    player->gtk_sink = gst_element_factory_make("gtk4paintablesink", "sink");

    //in audio, the queue, if the audio bit is high,
    //then converting to a streamable audio, resampling
    //and a sink for the audio to pair together with another sink
    player->audio_entry = gst_element_factory_make("queue", "a_queue");
    GstElement *a_conv = gst_element_factory_make("audioconvert", "a_conv");
    GstElement *a_res = gst_element_factory_make("audioresample", "a_res"); // ADD THIS
    GstElement *a_sink = gst_element_factory_make("autoaudiosink", "a_sink");

    //then, a volume made to account for app volume.
    //GstElement *a_scale = gst_element_factory_make("scaletempo", "a_scale");
    player->volume_stream = gst_element_factory_make("volume", "vol");

    // let the audio sink try to synchronize,
    //also, let the video sink try to synchronize and allocate to account for high bitrates
    //500 *GST_MSECOND = 500 milliseconds
    g_object_set(a_sink, "sync", TRUE, NULL);
    g_object_set(player->gtk_sink,
                 "sync", TRUE,
                 "qos", TRUE,
                 "max-lateness", 500 * GST_MSECOND,
                 "processing-deadline", 0,
                 "async", TRUE,
                 NULL);

    //buffering the audio just in case if there should be any dropout
    g_object_set(player->audio_entry,
                 "max-size-time", 2 * GST_SECOND,
                 "max-size-bytes", 0,
                 "max-size-buffers", 0, NULL);

    //an audio quality optimized to account for cpu
    g_object_set(a_res, "quality", 0, NULL);

    //we cap the video and constrain them into the resolution and format to output safely
    //format=BGRx
    GstCaps *v_caps;
    player->accel_type="";

    //focusing primarily on software acceleration for now
    if (gst_element_factory_find("vaapipostproc")) {
        v_conv = gst_element_factory_make("vaapipostproc", "v_conv");
        v_scale = NULL; // vaapipostproc handles scaling
        v_caps = gst_caps_from_string("video/x-raw, format=BGRx, width=1280, height=720");
        player->accel_type ="AMD/Intel";
    }
    // assume software
    else {
        v_conv = gst_element_factory_make("videoconvert", "v_conv");
        v_scale = gst_element_factory_make("videoscale", "v_scale");
        v_caps = gst_caps_from_string("video/x-raw, format=BGRx, width=1280, height=720");
        player->accel_type ="Software";
    }

    //caps
    g_object_set(v_limit, "caps", v_caps, NULL);
    gst_caps_unref(v_caps);

    //group the elements into a bin
    gst_bin_add_many(GST_BIN(player->pipeline), src, dbin,
                     player->video_entry, v_conv, v_scale, v_limit, player->gtk_sink,
                     player->audio_entry, a_conv, a_res, player->volume_stream, a_sink, NULL);

    //static link the elements in a bin
    gst_element_link(src, dbin);
    gst_element_link_many(player->video_entry, v_conv, v_scale, v_limit, player->gtk_sink, NULL);
    gst_element_link_many(player->audio_entry, a_conv, a_res, player->volume_stream, a_sink, NULL);

    //dynamically link the pads from function on_pad_added
    g_signal_connect(dbin, "pad-added", G_CALLBACK(on_pad_added), player);

    g_object_set(src, "location", path, NULL);

    //print detection
    g_print("%s\n", player->accel_type);
}


//reverts back to the beginning of a video
void seek_begin(VideoPlayer *player, gint64 pos_ns) {
    gst_element_seek_simple(player->pipeline,
                            GST_FORMAT_TIME,
                            GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_AFTER,
                            pos_ns);
}

//starts playback
gboolean start_playback(VideoPlayer *player) {
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
    g_print("New Speed: %.2f (%s)\n",
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
void change_volume(VideoPlayer *player, gdouble direction) {
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
    g_print("Volume: %.0f%%\n", current_vol * 100);
}

void seek_frames(VideoPlayer *player, gint64 direct_count) {
    gint64 current_pos;
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &current_pos)) return;

    double fps = player->assignedFPS;
    gint64 frame_duration = (gint64)(GST_SECOND / fps);

    // 1. Calculate the target in nanoseconds (for GStreamer)
    gint64 snapped_current = (current_pos / frame_duration) * frame_duration;
    gint64 target = snapped_current + (direct_count * frame_duration);
    if (target < 0) target = 0;

    // 2. Calculate the "Display Frame" (adjusted for drop-frame)
    gint64 display_frame = target / frame_duration;
    char separator = ':';

    if (fps > 29.9 && fps < 30.0) {
        separator = ';';
        gint64 d = display_frame / 17982;
        gint64 m = display_frame % 17982;
        display_frame += (18 * d) + 2 * ((m - 2) / 1798);
    } else if (fps > 59.9 && fps < 60.0) {
        separator = ';';
        gint64 d = display_frame / 35964;
        gint64 m = display_frame % 35964;
        display_frame += (36 * d) + 4 * ((m - 4) / 1798);
    }

    // 3. Breakdown the adjusted frame count for the CLI
    int fps_int = (int)(fps + 0.5);
    guint f = (guint)(display_frame % fps_int);
    guint s = (guint)((display_frame / fps_int) % 60);
    guint m = (guint)((display_frame / (fps_int * 60)) % 60);
    guint h = (guint)(display_frame / (fps_int * 3600));

    g_print("FPS: %.2f | Jump: %ld frames | Seeking to: %02u:%02u:%02u%c%02u\n",
            fps, (long)direct_count, h, m, s, separator, f);
    //then, treat the target frame as if it was the beginning and let it run
    gst_element_seek(player->pipeline, 1.0, GST_FORMAT_TIME,
                     // Add SNAP_NEAREST and TRICKMODE
                     GST_SEEK_FLAG_FLUSH |
                     GST_SEEK_FLAG_ACCURATE |
                     GST_SEEK_FLAG_TRICKMODE,
                     GST_SEEK_TYPE_SET, target,
                     GST_SEEK_TYPE_NONE, -1);
}
