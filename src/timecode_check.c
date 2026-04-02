//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info

#include "../include/timecode_check.h"
#include <stdbool.h>

char* update_live_timecode(const VideoPlayer *player) {
    gint64 pos_ns;
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &pos_ns))
        return g_strdup("00:00:00:00");

    double fps = player->assignedFPS;
    if (fps <= 0) return g_strdup("00:00:00:00");

    // use position number, frame count, and fps*1000
    gint64 frame_idx = gst_util_uint64_scale(pos_ns, (int)(fps * 1000 + 0.5), 1000 * GST_SECOND);

    // check if 59; check if drop frame range
    bool is_high_fps = (fps > 50.0);
    bool is_drop = (fps > 29.9 && fps < 30.0) || (fps > 59.9 && fps < 60.0);

    if (is_drop) {
        //check if 59...
        int drop = is_high_fps ? 4 : 2;
        // check total frames in 10 minutes
        gint64 chunk_10m = is_high_fps ? 35964 : 17982;
        // check total frames in 1 minute
        gint64 chunk_1m  = is_high_fps ? 3596 : 1798;

        gint64 D = frame_idx / chunk_10m;
        gint64 M = frame_idx % chunk_10m;

        //drop frames per minute except every 10 minutes
        if (M >= drop) {
            frame_idx += (9 * drop * D) + drop * ((M - drop) / chunk_1m);
        } else {
            frame_idx += (9 * drop * D);
        }
    }

    // prepare for timecode display
    int fps_int = (int)(fps + 0.5);
    int f = (int)(frame_idx % fps_int);
    int s = (int)((frame_idx / fps_int) % 60);
    int m = (int)((frame_idx / (fps_int * 60)) % 60);
    int h = (int)(frame_idx / (fps_int * 3600));

    char sep = is_drop ? ';' : ':';
    return g_strdup_printf("%02d:%02d:%02d%c%02d", h, m, s, sep, f);
}
