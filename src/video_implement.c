//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info


#include "../include/video_implement.h"

#include <stdio.h>

static GstBusSyncReply bus_sync_handler(GstBus *bus, GstMessage *message, gpointer user_data) {
    //make user_data accept VideoPlayer
    VideoPlayer *player = (VideoPlayer *) user_data;

    //if message is displayed
    if (gst_is_video_overlay_prepare_window_handle_message(message)) {
        //if window_id not 0
        if (player->window_id != 0) {
            // GStreamer 1.12+ helper to find the right object
            GstVideoOverlay *overlay = GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message));
            gst_video_overlay_set_window_handle(overlay, (guintptr) player->window_id);
        }
        //drop message
        return GST_BUS_DROP;
    }
    //pass message, else.
    return GST_BUS_PASS;
}

//on pad added
void on_pad_added(GstElement *src, GstPad *new_pad, gpointer data) {
    VideoPlayer *player = (VideoPlayer *) data;

    //make sure to leverage threads
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(src), "max-threads")) {
        g_object_set(src, "max-threads", 0, NULL);
    }

    // detect using query caps
    GstCaps *caps = gst_pad_query_caps(new_pad, NULL);
    GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(str);

    GstPad *sink_pad = NULL;

    // match string in video and audio
    if (g_str_has_prefix(name, "video/")) {
        //obtain pad from video queue
        sink_pad = gst_element_get_static_pad(player->video_entry, "sink");
    } else if (g_str_has_prefix(name, "audio/")) {

        //account for varying audio streams
        int pad_num = player->audio_pad_count++;
        int want = (player->selected_audio_stream <= 0)
                       ? 0
                       : player->selected_audio_stream;
        if (pad_num == want) {
            sink_pad = gst_element_get_static_pad(player->audio_entry, "sink");
        }
    }

    // link the pad, check if it's good
    if (sink_pad) {
        if (!gst_pad_is_linked(sink_pad)) {
            GstPadLinkReturn ret = gst_pad_link(new_pad, sink_pad);
            if (ret != GST_PAD_LINK_OK) {
                g_printerr("Failed to link %s pad. Error: %s\n",
                           name, gst_pad_link_get_name(ret));
            } else {
                g_print("Successfully linked %s pad\n", name);
            }
        }
        gst_object_unref(sink_pad);
    }
    //after finishing
    if (caps) gst_caps_unref(caps);
}

static VideoSinkType detect_best_sink(void) {
#ifdef __APPLE__
    // use osxvideosink ... else software acceleration
    if (gst_element_factory_find("osxvideosink")) {
        return SINK_OSX;
    }
#endif
    // prefer GL for linux
    GstElement *test_sink = gst_element_factory_make("glimagesink", "test_sink");
    if (test_sink) {
        GstStateChangeReturn ret = gst_element_set_state(test_sink, GST_STATE_READY);
        gst_element_set_state(test_sink, GST_STATE_NULL);
        gst_object_unref(test_sink);
        if (ret != GST_STATE_CHANGE_FAILURE) return SINK_GL;
    }

    //fallback for Apple hardware
#ifndef __APPLE__
    GstElementFactory *xv = gst_element_factory_find("xvimagesink");
    if (xv) {
        gst_object_unref(xv);
        return SINK_XV;
    }
#endif
//assign software if nothing works
    return SINK_SOFTWARE;
}


void init_video_processor(VideoPlayer *player, const char *path) {
    //reset to normal speed, first audio stream, declare pipeline
    player->current_rate = 1.0;
    player->audio_pad_count = 0;
    player->pipeline = gst_pipeline_new("qt-pipeline");

    //bus handler, make the player agree with the window in some way
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(player->pipeline));
    gst_bus_set_sync_handler(bus, bus_sync_handler, player, NULL);
    gst_object_unref(bus);

    VideoSinkType sink_type;

    if (player->use_hw_accel == -1) {
        sink_type = detect_best_sink();
        //prefer to use GL, if not, then use Metal, assuming computer is apple
        player->use_hw_accel = (sink_type == SINK_GL || sink_type == SINK_OSX) ? 1 : 0;
    } else {
        // user override, preferred sink based on platform
        if (player->use_hw_accel) {
#ifdef __APPLE__
            GstElementFactory *f = gst_element_factory_find("osxvideosink");
            if (f) {
                gst_object_unref(f);
                sink_type = SINK_OSX;
            } else sink_type = SINK_GL;
#else
            sink_type = SINK_GL;
#endif
        } else {
#ifdef __APPLE__
            sink_type = SINK_SOFTWARE;
#else
            sink_type = SINK_XV;
#endif
        }
    }

    // set accel_type label for display
    switch (sink_type) {
        case SINK_GL: player->accel_type = "GL";
            break;
        case SINK_OSX: player->accel_type = "OSX/Metal";
            break;
        case SINK_XV: player->accel_type = "Software+XV";
            break;
        case SINK_SOFTWARE: player->accel_type = "Software";
            break;
    }
    g_print("Sink: %s\n", player->accel_type);

    // file src -> decoding for bin
    GstElement *src = gst_element_factory_make("filesrc", "src");
    GstElement *dbin = gst_element_factory_make("decodebin", "dbin");
    g_object_set(src, "location", path, NULL);

    // video: queue -> convert
    player->video_entry = gst_element_factory_make("queue", "v_queue");
    GstElement *v_conv = gst_element_factory_make("videoconvert", "v_conv");
    GstElement *v_out_queue = NULL;
    GstElement *v_upload = NULL;
    GstElement *v_color = NULL;

    //create video sink based on hardware
    //gl uses upload->color convert -> glimagesink
    if (sink_type == SINK_GL) {
        v_upload = gst_element_factory_make("glupload", "v_upload");
        v_color = gst_element_factory_make("glcolorconvert", "v_color");
        player->video_sink = gst_element_factory_make("glimagesink", "sink");
        g_object_set(player->video_sink,
                     "sync", TRUE,
                     "handle-events", FALSE,
                     "async", TRUE,
                     "max-lateness", -1,
                     "show-preroll-frame", TRUE, NULL);
        //directly to video_sink osxvideosink as metal handles everything
    } else if (sink_type == SINK_OSX) {
        player->video_sink = gst_element_factory_make("osxvideosink", "sink");
        g_object_set(player->video_sink,
                     "sync", TRUE, "force-aspect-ratio", TRUE, NULL);
    } else {
        //let software have a sink
        v_out_queue = gst_element_factory_make("queue", "v_out_queue");
        g_object_set(v_out_queue,
                     "leaky", 0,
                     "max-size-buffers", 2,
                     "max-size-time", 0,
                     "max-size-bytes", 0, NULL);
        //check if software sink is possible, else automatically choose sink
        player->video_sink = (sink_type == SINK_XV)
                                 ? gst_element_factory_make("xvimagesink", "sink")
                                 : gst_element_factory_make("autovideosink", "sink");
        //configure the sink to be slightly faster in response
        g_object_set(player->video_sink,
                     "sync", TRUE,
                     "handle-events", FALSE,
                     "max-lateness", (gint64) 2 * GST_SECOND, NULL);
    }

    // stack queue -> convert -> resample -> volume -> sink
    player->audio_entry = gst_element_factory_make("queue", "a_queue");
    GstElement *a_dec_conv = gst_element_factory_make("audioconvert", "a_dec_conv");
    GstElement *a_conv = gst_element_factory_make("audioconvert", "a_conv");
    GstElement *a_res = gst_element_factory_make("audioresample", "a_res");
    player->volume_stream = gst_element_factory_make("volume", "vol");
    GstElement *a_sink = gst_element_factory_make("autoaudiosink", "a_sink");

    // allow room for video and audio queues to load
    g_object_set(player->audio_entry,
                 "max-size-time", (guint64) 2 * GST_SECOND,
                 "max-size-bytes", 0,
                 "max-size-buffers", 0, NULL);
    g_object_set(player->video_entry,
                 "max-size-time", (guint64) 3 * GST_SECOND,
                 "max-size-bytes", 0,
                 "max-size-buffers", 0,
                 "leaky", 0, NULL);

    //set the audio settings to synchronize and have HQ
    g_object_set(a_sink, "sync", TRUE, NULL);
    g_object_set(a_res, "quality", 0, NULL);

    // make bin from elements in pipeline
    if (sink_type == SINK_GL) {
        gst_bin_add_many(GST_BIN(player->pipeline),
                         src, dbin,
                         player->video_entry, v_conv, v_upload, v_color, player->video_sink,
                         player->audio_entry, a_dec_conv, a_conv, a_res,
                         player->volume_stream, a_sink, NULL);
    } else if (sink_type == SINK_OSX) {
        gst_bin_add_many(GST_BIN(player->pipeline),
                         src, dbin,
                         player->video_entry, v_conv, player->video_sink,
                         player->audio_entry, a_dec_conv, a_conv, a_res,
                         player->volume_stream, a_sink, NULL);
    } else {
        gst_bin_add_many(GST_BIN(player->pipeline),
                         src, dbin,
                         player->video_entry, v_conv, v_out_queue, player->video_sink,
                         player->audio_entry, a_dec_conv, a_conv, a_res,
                         player->volume_stream, a_sink, NULL);
    }

    // linking chains
    gst_element_link(src, dbin);

    //dynamically linking video
    if (sink_type == SINK_GL) {
        gst_element_link_many(player->video_entry, v_conv,
                              v_upload, v_color, player->video_sink, NULL);
    } else if (sink_type == SINK_OSX) {
        gst_element_link_many(player->video_entry, v_conv,
                              player->video_sink, NULL);
    } else {
        GstCaps *xv_caps = gst_caps_from_string(
            "video/x-raw, format=YV12, pixel-aspect-ratio=1/1"
        );
        gst_element_link(player->video_entry, v_conv);
        gst_element_link_filtered(v_conv, v_out_queue, xv_caps);
        gst_element_link(v_out_queue, player->video_sink);
        gst_caps_unref(xv_caps);
    }

    //dynamically linking in audio
    gst_element_link_many(player->audio_entry, a_dec_conv, a_conv,
                          a_res, player->volume_stream, a_sink, NULL);

    //add to pad
    g_signal_connect(dbin, "pad-added", G_CALLBACK(on_pad_added), player);
}

void set_video_window(VideoPlayer *player, unsigned long window_id) {
    //assign window id for use by qt
    player->window_id = window_id;

    //check if video sink exists
    if (player->video_sink) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(player->video_sink), (guintptr) window_id);
    }
}

//show black screen
void init_idle_pipeline(VideoPlayer *player) {
    player->pipeline = gst_pipeline_new("idle-pipeline");

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(player->pipeline));
    gst_bus_set_sync_handler(bus, bus_sync_handler, player, NULL);
    gst_object_unref(bus);

    //set black screen
    GstElement *src = gst_element_factory_make("videotestsrc", "src");
    g_object_set(src, "pattern", 2, NULL);

    //detect appropriate sink based on hardware/software
    VideoSinkType sink_type = detect_best_sink();

    GstElement *sink = NULL;
    GstElement *v_upload = NULL;
    GstElement *v_color = NULL;

    //creates pipeline according to gpu/cpu
    if (sink_type == SINK_GL) {
        v_upload = gst_element_factory_make("glupload", "v_upload");
        v_color = gst_element_factory_make("glcolorconvert", "v_color");
        sink = gst_element_factory_make("glimagesink", "sink");
        g_object_set(sink, "sync", TRUE, "handle-events", FALSE, NULL);
#ifdef __APPLE__
    } else if (sink_type == SINK_OSX) {
            sink = gst_element_factory_make("osxvideosink", "sink");
            g_object_set(sink, "sync", TRUE, NULL);
#endif
    } else {
        sink = (sink_type == SINK_XV)
                   ? gst_element_factory_make("xvimagesink", "sink")
                   : gst_element_factory_make("autovideosink", "sink");
        g_object_set(sink, "sync", TRUE, "handle-events", FALSE, NULL);
    }

    //then, set the sink for now
    player->video_sink = sink;

    if (sink_type == SINK_GL) {
        gst_bin_add_many(GST_BIN(player->pipeline),
                         src, v_upload, v_color, sink, NULL);
        gst_element_link_many(src, v_upload, v_color, sink, NULL);
    } else {
        gst_bin_add_many(GST_BIN(player->pipeline), src, sink, NULL);
        gst_element_link(src, sink);
    }
}
