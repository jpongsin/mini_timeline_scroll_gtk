// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#ifndef TIMELINE_SCRUB_QT_VIDEO_WINDOW_H
#define TIMELINE_SCRUB_QT_VIDEO_WINDOW_H

#include <QtWidgets>

#include "video_implement.h"
#include "video_hotkey_actions.h"
#include "timecode_check.h"

class VideoWindow;

//links to video_window.cpp
extern"C" {
#include <libavformat/avformat.h>
#include <gst/video/videooverlay.h>
}

// VideoSurface: plain native widget reserved entirely for GStreamer
class VideoSurface : public QWidget {
public:
    explicit VideoSurface(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_DontCreateNativeAncestors);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setUpdatesEnabled(false);
        winId();
    }
};


bool handle_hotkeys(QKeyEvent *event, VideoPlayer *player, VideoWindow *window);

class VideoWindow : public QWidget {
public:
    VideoPlayer player;

    VideoWindow(int argc, char *argv[], QWidget *parent = nullptr);
    ~VideoWindow();

    void init_screen();
    void mainFileMenu(QMenuBar *menuBar);
    void zoomViewMenu(QMenuBar *menuBar);
    void playbackMenu(QMenuBar *menuBar);
    void helpMenu(QMenuBar *menuBar);

    void open_file_dialog();
    void probeAudio(AVFormatContext *streamCtx);
    void load_new_video(const QString &fileName);
    void close_recent_video();
    void export_screenshot();

    void toggleFullscreen();
    void toggleAutohide();

    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool isFullscreenActive;
    bool isAutohideActive;

    void showUI();   // restore all UI elements, stop hide timer
    void hideUI();   // hide menubar + toolbar + statusLabel, keep timecode

    // forces GStreamer to redraw into the correct rectangle after transitions
    void refreshVideoRect();

    QLabel       *statusLabel;
    QAction      *closeVideoAction;
    QVBoxLayout  *layout;
    VideoSurface *videoSurface;
    QLabel       *timecodeLabel;
    QMenuBar     *menuBar;

    // toolbar groups audioBox + hwAccelBox for autohide function
    QWidget      *toolbar;
    QComboBox    *audioBox;
    QCheckBox    *hwAccelBox;

    // auto-hide timer: fires hideUI() after 3s of mouse inactivity
    QTimer       *hideTimer;
};

#endif
