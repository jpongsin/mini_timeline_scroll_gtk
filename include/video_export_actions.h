//
// Created by jop on 4/12/26.
//

#ifndef TIMELINE_SCRUB_QT_EXPORT_ACTIONS_H
#define TIMELINE_SCRUB_QT_EXPORT_ACTIONS_H

#include "video_implement.h"

#ifdef __cplusplus
extern "C" {
#endif

//retrieves the paused or about to be paused frame before exporting.
GstSample *capture_current_frame(const VideoPlayer *player);

//result codes
typedef enum {
    EXPORT_OK = 0,
    EXPORT_ERR_CAPS,          // could not read width/height/format from caps
    EXPORT_ERR_MAP,           // gst_buffer_map failed
    EXPORT_ERR_FORMAT,        // av_guess_format returned NULL
    EXPORT_ERR_CONTEXT,       // avformat_alloc_output_context2 failed
    EXPORT_ERR_NO_ENCODER,    // avcodec_find_encoder returned NULL
    EXPORT_ERR_NO_STREAM,     // avformat_new_stream failed
    EXPORT_ERR_OPEN_ENCODER,  // avcodec_open2 failed
    EXPORT_ERR_OPEN_FILE,     // avio_open failed
    EXPORT_ERR_ENCODE,        // send_frame / receive_packet failed
    EXPORT_ERR_SWS,           // sws_getContext failed
} ExportResult;

//encodes frame to disk
//pixel format negotiation
//converting and scaling
//cleanups and handling errors
ExportResult write_frame_to_file(GstSample *sample,
                                 const char *out_path,
                                 const char *ffmpeg_name);

// for use in returning the appropriate enum
const char *export_result_string(ExportResult result);
#ifdef __cplusplus
}
#endif
#endif //TIMELINE_SCRUB_QT_EXPORT_ACTIONS_H
