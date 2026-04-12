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

//links to video_window.cpp
extern"C" {
#include <libavformat/avformat.h>
#include <gst/video/videooverlay.h>
}

// video widget specifically made to adapt gstreamer's window with qt's
class VideoSurface : public QWidget {
public:
    explicit VideoSurface(QWidget *parent = nullptr) : QWidget(parent) {
        // keep the widget plain for it to be reserved by gstreamer
        setAttribute(Qt::WA_NoSystemBackground);
        this->setAttribute(Qt::WA_NativeWindow);
        this->setAttribute(Qt::WA_DontCreateNativeAncestors);
        setAttribute(Qt::WA_OpaquePaintEvent);
        this->setUpdatesEnabled(false);

        // assign windowID so gstreamer can obtain
        this->winId();
    }

};

// prototype for hotkeys functions
bool handle_hotkeys(QKeyEvent *event, VideoPlayer *player, QWidget *window);

// a whole Qt window
class VideoWindow : public QWidget {
    // Q_OBJECT
public:
    // constructors
    VideoPlayer player;

    VideoWindow(int argc, char *argv[], QWidget *parent = nullptr);

    // destructor
    ~VideoWindow();

    // void functions

    // dropdown menu
    void mainFileMenu(QMenuBar *menuBar);

    void playbackMenu(QMenuBar *menuBar);

    void helpMenu(QMenuBar *menuBar);

    // opens a dialog to open files
    void open_file_dialog();

    // for changing audio streams or loading another video
    void probeAudio(AVFormatContext *streamCtx);

    void load_new_video(const QString &fileName);
    void close_recent_video();


    //screenshot
    void export_screenshot();

    // let hotkeys be allowed in app
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // header title for video file
    QLabel *statusLabel;

    // qt widgets
    QAction *closeVideoAction;
    QVBoxLayout *layout;
    VideoSurface *videoSurface;
    QLabel *timecodeLabel;
    QComboBox *audioBox;
    QMenuBar *menuBar;
};

#endif
