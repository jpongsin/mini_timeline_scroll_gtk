// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info


#include "../include/video_export_actions.h"
#include "../include/video_window.h"
#include "../include/video_fetch.h"

// Constructor
VideoWindow::VideoWindow(int argc, char *argv[], QWidget *parent)
    : QWidget(parent) {

  // declare variables.
  player = {0};
  player.use_hw_accel = -1;
  player.selected_audio_stream = 0;

  // build window
  setWindowTitle("Mini Timeline View");
  resize(800, 600);
  layout = new QVBoxLayout(this);

  // Menu Bar
  menuBar = new QMenuBar(this);
  mainFileMenu(menuBar);
  playbackMenu(menuBar);
  helpMenu(menuBar);
  layout->setMenuBar(menuBar);

  // Change name and default text
  statusLabel = new QLabel("No file loaded. Open a video to begin.");
  statusLabel->setStyleSheet(
      "background-color: #222; color: #aaa; padding: 5px; font-style: italic;");
  layout->addWidget(statusLabel);

  // widget for video
  videoSurface = new VideoSurface(this);
  videoSurface->winId();
  videoSurface->setStyleSheet("background-color: black;");
  videoSurface->setMinimumHeight(300);
  layout->addWidget(videoSurface, 1);

  // timecode
  timecodeLabel = new QLabel("00:00:00:00");
  timecodeLabel->setAlignment(Qt::AlignCenter);
  timecodeLabel->setStyleSheet("font-family: monospace; font-size: 24pt; "
                               "color: white; background-color: #111;");
  layout->addWidget(timecodeLabel);

  // new audio track choice menu
  audioBox = new QComboBox(this);
  audioBox->addItem("No file loaded");
  audioBox->setStyleSheet(
      "background-color: #222; color: white; padding: 4px;");
  layout->addWidget(audioBox);
  audioBox->blockSignals(false);

  // obtain audio track
  connect(audioBox, &QComboBox::currentIndexChanged, [this](int index) {
    if (!audioBox->isEnabled())
      return;
    if (!player.uri)
      return;

    // get the AVStream index stored in item data
    player.selected_audio_stream = audioBox->itemData(index).toInt();

    // save position then reload
    gint64 saved_pos = 0;
    if (player.pipeline)
      gst_element_query_position(player.pipeline, GST_FORMAT_TIME, &saved_pos);

    // reload preserves uri and use_hw_accel, just rebuilds pipeline
    load_new_video(QString(player.uri));

    // restore position after pipeline settles
    QTimer::singleShot(400, [this, saved_pos]() {
      if (saved_pos > 0 && player.pipeline) {
        gst_element_seek_simple(
            player.pipeline, GST_FORMAT_TIME,
            (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
            saved_pos);
      }
    });
  });

  // hardware acceleration checkbox
  QCheckBox *hwAccelBox = new QCheckBox("Hardware Acceleration (auto-detect)");
  hwAccelBox->setChecked(true);
  hwAccelBox->setStyleSheet(
      "color: white; background-color: #111; padding: 4px;");
  layout->addWidget(hwAccelBox);
  connect(hwAccelBox, &QCheckBox::toggled, [this](bool checked) {
    player.use_hw_accel = checked ? 1 : 0;
    if (player.uri)
      load_new_video(QString(player.uri));
  });

  // timecode, connected to pipeline
  QTimer *tcTimer = new QTimer(this);
  connect(tcTimer, &QTimer::timeout, [this]() {
    // if pipeline active and has fps
    if (player.pipeline && player.assignedFPS > 0.0) {
      // make a new updating timecode
      char *tc = update_live_timecode(&player);
      // assign string
      QString newTc = QString(tc);
      if (tc && newTc != "00:00:00:00") {
        // check if video running
        timecodeLabel->setText(newTc);
      } else if (newTc == "00:00:00:00") {
        // if video not on any position
        gint64 pos;
        if (gst_element_query_position(player.pipeline, GST_FORMAT_TIME,
                                       &pos) &&
            pos == 0) {
          timecodeLabel->setText(newTc);
        }
      }
      if (tc)
        g_free(tc);
    }
  });
  // warm up
  tcTimer->start(30);

  // store the ID for GStreamer Sink
  player.window_id = (unsigned long)videoSurface->winId();

  // make black screen
  init_idle_pipeline(&player);

  QTimer::singleShot(100, [this]() {
    set_video_window(&player, player.window_id);
    gst_element_set_state(player.pipeline, GST_STATE_PLAYING);
  });

  // if using command line arguments
  if (argc > 1) {
    QTimer::singleShot(250,
                       [this, argv]() { load_new_video(QString(argv[1])); });
  }
}

// creates file menu
void VideoWindow::mainFileMenu(QMenuBar *menuBar) {
  QMenu *fileMenu = menuBar->addMenu(tr("&File"));

  QAction *openAction = fileMenu->addAction(tr("&Open Video..."));
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this,
          &VideoWindow::open_file_dialog);

  closeVideoAction = fileMenu->addAction(tr("&Close Video"));
  // disabled until a file is loaded
  closeVideoAction->setShortcut(QKeySequence::Close);
  closeVideoAction->setEnabled(false);
  connect(closeVideoAction, &QAction::triggered, this,
          &VideoWindow::close_recent_video);

  QAction *closeAction = fileMenu->addAction(tr("&Quit"));
  closeAction->setShortcut(QKeySequence::Quit);
  connect(closeAction, &QAction::triggered, this, &QWidget::close);
}

// create about menu
void VideoWindow::helpMenu(QMenuBar *menuBar) {
  QMenu *helpMenu = menuBar->addMenu(tr("&Help"));

  QAction *aboutAction = helpMenu->addAction(tr("&About..."));
  connect(aboutAction, &QAction::triggered, [this]() {
    // The "About" Pop-up logic
    QMessageBox::about(
        this, tr("About Mini Timeline View"),
        tr("<h3>Mini Timeline View</h3>"
           "<p>A basic keyboard controlled media player.</p>"
           "<p>© 2026 jpongsin</p>"
           "<p>Licensed under MIT License</p>"
           "<p> For more info, refer to README.md and LICENSE.md</p>"));
  });

  // add about Qt
  helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}
// create playback menu
void VideoWindow::playbackMenu(QMenuBar *menuBar) {
  QMenu *playMenu = menuBar->addMenu(tr("&Playback"));

  QAction *playAction = playMenu->addAction(tr("Play/Pause"));
  playAction->setShortcut(QKeySequence(Qt::Key_Space));
  connect(playAction, &QAction::triggered,
          [this]() { toggle_playback(&player); });

  playMenu->addSeparator();

  QAction *forwardAction = playMenu->addAction(tr("Step Forward (10 frames)"));
  forwardAction->setShortcut(QKeySequence(Qt::Key_L));
  connect(forwardAction, &QAction::triggered,
          [this]() { seek_mechanism(&player, 10); });

  QAction *backAction = playMenu->addAction(tr("Step Backward (10 frames)"));
  backAction->setShortcut(QKeySequence(Qt::Key_J));
  connect(backAction, &QAction::triggered,
          [this]() { seek_mechanism(&player, -10); });

  playMenu->addSeparator();

  QAction *speedUpAction = playMenu->addAction(tr("Increase Speed (+0.25)"));
  speedUpAction->setShortcut(QKeySequence(Qt::Key_BracketRight));
  connect(speedUpAction, &QAction::triggered,
          [this]() { change_rate(&player, 0.25); });

  QAction *speedDownAction = playMenu->addAction(tr("Decrease Speed (-0.25)"));
  speedDownAction->setShortcut(QKeySequence(Qt::Key_BracketLeft));
  connect(speedDownAction, &QAction::triggered,
          [this]() { change_rate(&player, -0.25); });

  playMenu->addSeparator();

  QAction *volUpAction = playMenu->addAction(tr("Increase Volume (+0.10)"));
  volUpAction->setShortcut(QKeySequence(Qt::Key_Up));
  connect(volUpAction, &QAction::triggered,
          [this]() { change_volume(&player, 0.10); });

  QAction *volDownAction = playMenu->addAction(tr("Decrease Volume (-0.10)"));
  volDownAction->setShortcut(QKeySequence(Qt::Key_Down));
  connect(volDownAction, &QAction::triggered,
          [this]() { change_volume(&player, -0.10); });

  playMenu->addSeparator();

  QAction *homeAction = playMenu->addAction(tr("Go to Start"));
  homeAction->setShortcut(QKeySequence(Qt::Key_Home));
  connect(homeAction, &QAction::triggered,
          [this]() { seek_begin(&player, 0); });

  QAction *screenshotAction = playMenu->addAction(tr("&Save Screenshot..."));
  screenshotAction->setShortcut(QKeySequence(Qt::Key_S));
  connect(screenshotAction, &QAction::triggered,
          this, &VideoWindow::export_screenshot);

}

// destructor
VideoWindow::~VideoWindow() {
  if (player.pipeline) {
    gst_element_set_state(player.pipeline, GST_STATE_NULL);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(player.pipeline));
    gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
    gst_object_unref(bus);
    gst_object_unref(player.pipeline);
  }
  if (player.uri) {
    free((void *)player.uri);
    player.uri = nullptr;
  }
  gst_deinit();
}

// try to resize
void VideoWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (player.video_sink && videoSurface) {
    gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(player.video_sink),
                                           0, 0, videoSurface->width(),
                                           videoSurface->height());
    gst_video_overlay_expose(GST_VIDEO_OVERLAY(player.video_sink));
  }
}

// show that hotkeys are permitted while cursor is above screen
bool VideoWindow::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if (handle_hotkeys(keyEvent, &player, this)) {
      return true;
    }
  }
  return QWidget::eventFilter(obj, event);
}

bool handle_hotkeys(QKeyEvent *event, VideoPlayer *player, QWidget *window) {
  if (QApplication::activeModalWidget() != nullptr)
    return false;
  int key = event->key();

  switch (key) {
  // step down ten frames
  case Qt::Key_J:
    seek_mechanism(player, -10);
    return true;
    // step up ten frames
  case Qt::Key_L:
    seek_mechanism(player, 10);
    return true;
    // play/pause
  case Qt::Key_K:
  case Qt::Key_Space: {
    toggle_playback(player);
    return true;
  }
      //snapshot
    case Qt::Key_S: {
    capture_current_frame(player);
    return true;
    }
    // step one down frame
  case Qt::Key_Left:
    on_key_left(player);
    return true;
    // step one up frame
  case Qt::Key_Right:
    on_key_right(player);
    return true;
    // toggle fullscreen mode
  case Qt::Key_F:
    if (window->isFullScreen())
      window->showNormal();
    else
      window->showFullScreen();
    return true;
    // rewind back home to beginning
  case Qt::Key_Home:
    seek_begin(player, 0);
    return true;
    // mute audio
  case Qt::Key_M: {
    toggle_mute(player);
    return true;
  }
    // brackets: Speed
  case Qt::Key_BracketRight:
    change_rate(player, 0.25);
    return true;
  case Qt::Key_BracketLeft:
    change_rate(player, -0.25);
    return true;

    // arrows: volume
  case Qt::Key_Up:
    change_volume(player, 0.10);
    return true;
  case Qt::Key_Down:
    change_volume(player, -0.10);
    return true;
  default:
    return false;
  }
}
// #include "video_window.moc"
