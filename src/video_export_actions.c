//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info

#include "../include/video_export_actions.h"
#include "../include/video_hotkey_actions.h"

#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

//pauses pipeline unconditionally, then check if it's part of pipeline
//empty pipeline = no screenshots
GstSample *capture_current_frame(const VideoPlayer *player) {
    //safe function to keep from crashing
    if (!pipeline_is_active(player)) return NULL;

    GstState current, pending;

    //get last frame
    gst_element_get_state(player->pipeline, &current, &pending, 0);
    //put a pipeline to a pause
    if (current == GST_STATE_PLAYING || pending == GST_STATE_PLAYING) {
        gst_element_set_state(player->pipeline, GST_STATE_PAUSED);
        g_print("\rFrame frozen for capture.");
        fflush(stdout);
    }

    //handle if no video available or still loading
    GstSample *sample = NULL;
    g_object_get(player->video_sink, "last-sample", &sample, NULL);
    if (!sample)
        g_printerr("capture_current_frame: no sample available from sink.\n");

    return sample;
}

//result string
//for logging possible errors
const char *export_result_string(ExportResult result) {
    switch (result) {
        case EXPORT_OK:               return "OK";
        case EXPORT_ERR_CAPS:         return "Could not read frame dimensions from caps.";
        case EXPORT_ERR_MAP:          return "Could not map frame buffer.";
        case EXPORT_ERR_FORMAT:       return "FFmpeg does not recognise this output format.";
        case EXPORT_ERR_CONTEXT:      return "Could not create FFmpeg output context.";
        case EXPORT_ERR_NO_ENCODER:   return "No FFmpeg encoder found for this format.";
        case EXPORT_ERR_NO_STREAM:    return "Could not allocate output stream.";
        case EXPORT_ERR_OPEN_ENCODER: return "Could not open encoder.";
        case EXPORT_ERR_OPEN_FILE:    return "Could not open output file for writing.";
        case EXPORT_ERR_ENCODE:       return "Encoding failed.";
        case EXPORT_ERR_SWS:          return "Pixel format conversion failed (swscale).";
        default:                      return "Unknown export error.";
    }
}

//write frame to file
//mostly ffmpeg exports
ExportResult write_frame_to_file(GstSample *sample,
                                 const char *out_path,
                                 const char *ffmpeg_name) {
    // extract caps
    GstBuffer    *buffer = gst_sample_get_buffer(sample);
    GstCaps      *caps   = gst_sample_get_caps(sample);
    GstStructure *s      = caps ? gst_caps_get_structure(caps, 0) : NULL;

    gint width = 0, height = 0;
    const gchar *format_str = NULL;

    if (!s ||
        !gst_structure_get_int(s, "width",  &width)  ||
        !gst_structure_get_int(s, "height", &height) ||
        !(format_str = gst_structure_get_string(s, "format")) ||
        width <= 0 || height <= 0) {
        return EXPORT_ERR_CAPS;
    }

    //map buffer
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ))
        return EXPORT_ERR_MAP;

    //map GStreamer pixel format -> AVPixelFormat
    enum AVPixelFormat src_fmt = AV_PIX_FMT_BGR0;
    if      (!g_strcmp0(format_str, "RGBA")) src_fmt = AV_PIX_FMT_RGBA;
    else if (!g_strcmp0(format_str, "RGB"))  src_fmt = AV_PIX_FMT_RGB24;
    else if (!g_strcmp0(format_str, "I420")) src_fmt = AV_PIX_FMT_YUV420P;
    else if (!g_strcmp0(format_str, "NV12")) src_fmt = AV_PIX_FMT_NV12;
    else if (!g_strcmp0(format_str, "YV12")) src_fmt = AV_PIX_FMT_YUV420P;

    ExportResult result = EXPORT_OK;

    //FFmpeg setup
    const AVOutputFormat  *out_fmt  = av_guess_format(ffmpeg_name, out_path, NULL);
    AVFormatContext       *fmt_ctx  = NULL;
    AVCodecContext        *codec_ctx = NULL;
    AVFrame               *frame    = NULL;
    AVPacket              *pkt      = NULL;
    struct SwsContext     *sws      = NULL;

    if (!out_fmt) {
        result = EXPORT_ERR_FORMAT;
        goto cleanup_map;
    }

    if (avformat_alloc_output_context2(&fmt_ctx, out_fmt, NULL, out_path) < 0) {
        result = EXPORT_ERR_CONTEXT; goto cleanup_map;
    }

    const AVCodec *codec = avcodec_find_encoder(out_fmt->video_codec);
    if (!codec) {
        result = EXPORT_ERR_NO_ENCODER;
        goto cleanup_fmt;
    }

    AVStream *stream = avformat_new_stream(fmt_ctx, codec);
    if (!stream) {
        result = EXPORT_ERR_NO_STREAM;
        goto cleanup_fmt;
    }

    //negotiate pixel format
    enum AVPixelFormat codec_pix_fmt = AV_PIX_FMT_RGB24;
    if (codec->pix_fmts) {
        codec_pix_fmt = avcodec_find_best_pix_fmt_of_list(
            codec->pix_fmts, src_fmt, 0, NULL);
    }

    //prepare for scaling
    codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = (AVRational){1, 25};
    codec_ctx->pix_fmt = codec_pix_fmt;

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        result = EXPORT_ERR_OPEN_ENCODER;
        goto cleanup_codec;
    }

    avcodec_parameters_from_context(stream->codecpar, codec_ctx);

    if (!(out_fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmt_ctx->pb, out_path, AVIO_FLAG_WRITE) < 0) {
            result = EXPORT_ERR_OPEN_FILE;
            goto cleanup_codec;
        }
    }

    if (avformat_write_header(fmt_ctx, NULL) < 0)
        g_printerr("Warning: avformat_write_header failed, continuing.\n");

    //use scaling to go from src format to codec formating
    sws = sws_getContext(width, height, src_fmt,
                         width, height, codec_pix_fmt,
                         SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws) {
        result = EXPORT_ERR_SWS;
        goto cleanup_file;
    }

    frame = av_frame_alloc();
    frame->width = width;
    frame->height = height;
    frame->format = codec_pix_fmt;
    av_frame_get_buffer(frame, 0);

    const uint8_t *src_slices[1] = {map.data};
    int src_stride[1] = {(int) map.size / height};
    sws_scale(sws, src_slices, src_stride, 0, height,
              frame->data, frame->linesize);
    sws_freeContext(sws);
    sws = NULL;

    pkt = av_packet_alloc();

    //encoding
    frame->pts = 0;
    if (avcodec_send_frame(codec_ctx, frame) == 0) {
        // receive the packet, first attempt
        int recv = avcodec_receive_packet(codec_ctx, pkt);

        if (recv == AVERROR(EAGAIN)) {
            //two step for libwebp encoding; flush with null
            avcodec_send_frame(codec_ctx, NULL);
            recv = avcodec_receive_packet(codec_ctx, pkt);
        }

        //packet...
        if (recv == 0) {
            av_interleaved_write_frame(fmt_ctx, pkt);
            result = EXPORT_OK;
        }
    }

    av_write_trailer(fmt_ctx);

cleanup_file:{
        if (!(out_fmt->flags & AVFMT_NOFILE))
            avio_closep(&fmt_ctx->pb);
    }
cleanup_codec:{
        avcodec_free_context(&codec_ctx);
    }
cleanup_fmt:{
    if (frame)   av_frame_free(&frame);
    if (pkt)     av_packet_free(&pkt);
    if (sws)     sws_freeContext(sws);
    avformat_free_context(fmt_ctx);
}
cleanup_map:{
    gst_buffer_unmap(buffer, &map);
    //only logged on command line; no error msg dialog for GUI
    if (result == EXPORT_OK)
        g_print("\rScreenshot saved: %s", out_path);
    else
        g_printerr("\rExport error: %s\n", export_result_string(result));

    fflush(stdout);
    return result;
}
}
