// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/video_export_actions.h"
#include "../include/video_window.h"
#include "../include/video_fetch.h"

// open file dialog
void VideoWindow::open_file_dialog() {
    QStringList video_extensions = {"mp4", "mkv", "mov", "avi", "webm", "flv",
                                    "vob", "ogv", "ogg", "wmv", "mpg",  "mpeg",
                                    "m4v", "3gp", "3g2", "mxf", "f4v"};

    QString filter = "Video Files (";
    for (const QString &ext : video_extensions) {
        filter += "*." + ext + " ";
    }
    filter += ");;All Files (*)";

    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open Video"), "", filter);

    if (!fileName.isEmpty()) {
        load_new_video(fileName);
    }
}
// instructions to load video
void VideoWindow::load_new_video(const QString &fileName) {
    // check existing pipeline, then destroy it
    if (player.pipeline) {
        gst_element_set_state(player.pipeline, GST_STATE_NULL);
        gst_object_unref(player.pipeline);
        player.pipeline = nullptr;
    }
    // clear uri
    if (player.uri) {
        free((void *)player.uri);
        player.uri = NULL;
    }

    // fetch from ffmpeg
    player.uri = strdup(fileName.toUtf8().constData());
    AVFormatContext *forIOContext = nullptr;
    player.assignedFPS = validateVideo(player.uri, forIOContext, -1);
    avformat_close_input(&forIOContext);

    // open audio streams and probe
    AVFormatContext *streamCtx = nullptr;
    probeAudio(streamCtx);

    // Update the label once the file is ready
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.fileName();

    // start up video processor and then label
    init_video_processor(&player, player.uri);
    statusLabel->setText(QString("File: %1 | FPS: %2 | Engine: %3")
                             .arg(baseName)
                             .arg(player.assignedFPS, 0, 'f', 2)
                             .arg(player.accel_type));

    statusLabel->setStyleSheet(
        "background-color: #333; color: #FFFFFF; padding: 5px;");

    // get startup pipeline
    set_video_window(&player, player.window_id);
    gst_element_set_state(player.pipeline, GST_STATE_READY);

    // with handler, allow window to settle before starting playback
    QTimer::singleShot(150, [this]() {
      start_playback(&player);
      this->setFocus();
    });
}


void VideoWindow::probeAudio(AVFormatContext *streamCtx) {
    if (avformat_open_input(&streamCtx, player.uri, NULL, NULL) == 0 &&
        avformat_find_stream_info(streamCtx, NULL) >= 0) {

        // clear any existing streams
        audioBox->blockSignals(true);
        audioBox->clear();
        audioBox->setEnabled(false);

        int audio_track = 0;
        for (unsigned int i = 0; i < streamCtx->nb_streams; i++) {
            if (streamCtx->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
                continue;

            AVDictionaryEntry *lang =
                av_dict_get(streamCtx->streams[i]->metadata, "language", NULL, 0);
            AVDictionaryEntry *title =
                av_dict_get(streamCtx->streams[i]->metadata, "title", NULL, 0);

            QString label;
            if (title && strlen(title->value) > 0)
                label = QString("%1").arg(title->value);
            else
                label = QString("Track %1").arg(audio_track + 1);

            if (lang && strlen(lang->value) > 0)
                label += QString(" (%1)").arg(lang->value);

            // store the AVStream index as item data
            // in load_new_video, change the addItem call
            audioBox->addItem(label, QVariant(audio_track));
            audio_track++;
        }

        if (audio_track > 0) {
            audioBox->setEnabled(true);

            // if previous selection exceeds available tracks, fall back to 0
            if (player.selected_audio_stream >= audio_track) {
                player.selected_audio_stream = 0;
            }

            // restore combobox to match clamped selection
            audioBox->blockSignals(true);
            audioBox->setCurrentIndex(player.selected_audio_stream);
            audioBox->blockSignals(false);
        } else {
            // no audio at all
            audioBox->addItem("No audio streams");
            audioBox->setEnabled(false);
            player.selected_audio_stream = 0;
        }
        audioBox->blockSignals(false);
        avformat_close_input(&streamCtx);
        }
}


//screenshot
void VideoWindow::export_screenshot() {
    //grab frame: determine if video pipeline available
    //whether playing or pausing, the frame is grabbed
    GstSample *sample = capture_current_frame(&player);
    if (!sample) {
        QMessageBox::warning(this, tr("Screenshot"),
                             tr("No frame available. Load a video first."));
        return;
    }

    //pick a format
    //label shown to user, file extension, ffmpeg short name
    struct ImageFormat {
        const char *label;
        const char *ext;
        const char *ffmpeg_name;
    };

    static const ImageFormat formats[] = {
        { "PNG  – Lossless, transparency support",          "png",  "png"   },
        { "JPEG – Lossy, widely compatible",                "jpg",  "mjpeg" },
        { "BMP  – Uncompressed bitmap",                     "bmp",  "bmp"   },
        { "TIFF – Lossless, professional use",              "tiff", "tiff"  },
        { "WebP – Modern lossy/lossless",                   "webp", "webp"  },
        { "PPM  – Simple portable pixmap",                  "ppm",  "ppm"   },
        { "DPX  – Digital cinema / VFX",   "dpx",  "dpx"   },
        { "EXR  – HDR / VFX",              "exr",  "exr"   },
    };
    const int format_count = (int)(sizeof(formats) / sizeof(formats[0]));

    QStringList format_labels;
    for (int i = 0; i < format_count; i++)
        format_labels << QString(formats[i].label);

    bool ok = false;
    QString chosen_label = QInputDialog::getItem(
        this,
        tr("Choose Image Format"),
        tr("Export format:"),
        format_labels,
        0,      // default: PNG
        false,  // not editable
        &ok
    );

  //disable all keys temporarily,
  //so keys can be free to navigate OK or cancel.
  //handled in handle_hotkeys

    if (!ok) {
        gst_sample_unref(sample);
        g_print("\rScreenshot cancelled.");
        fflush(stdout);
        return;
    }

    const ImageFormat *chosen_fmt = nullptr;
    for (int i = 0; i < format_count; i++) {
        if (chosen_label == QString(formats[i].label)) {
            chosen_fmt = &formats[i];
            break;
        }
    }
    if (!chosen_fmt) {
        gst_sample_unref(sample);
        return;
    }

    // save dialog
    QString ext  = QString(chosen_fmt->ext);
    QString home = QDir::homePath();

    QString default_name = QString("screenshot.") + ext;

    QString filter = QString("%1 Files (*.%2);;All Files (*)")
                         .arg(ext.toUpper()).arg(ext);

    QString outputPath = QFileDialog::getSaveFileName(
        this,
        tr("Save Screenshot"),
        home + "/" + default_name,
        filter
    );

    if (outputPath.isEmpty()) {
        gst_sample_unref(sample);
        g_print("\rScreenshot cancelled.");
        fflush(stdout);
        return;
    }

    //extension still has to be present after user deletes
    if (!outputPath.endsWith("." + ext, Qt::CaseInsensitive))
        outputPath += "." + ext;

    //call export image function in video_export_actions.c
    QByteArray pathBytes = outputPath.toUtf8();
    ExportResult result = write_frame_to_file(
        sample,
        pathBytes.constData(),
        chosen_fmt->ffmpeg_name
    );

    gst_sample_unref(sample);

    // show error if not succeeded
    // else, write_frame_to_file handles completion
    if (result != EXPORT_OK) {
        QMessageBox::warning(this, tr("Screenshot"),
                             tr(export_result_string(result)));
    }
}
