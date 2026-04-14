// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/video_window.h"
#include "../include/video_export_actions.h"
#include "../include/video_fetch.h"
#include "../include/video_viewscaling.h"

VideoWindow::VideoWindow(int argc, char *argv[], QWidget *parent){
  // initialize for preparing fullscreen and immersive mode
  init_screen();

  player = {0};
  player.use_hw_accel = -1;
  player.selected_audio_stream = 0;

  setWindowTitle("Mini Timeline View");
  resize(800, 600);
  setMouseTracking(true);

  layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  menuBar = new QMenuBar(this);
  mainFileMenu(menuBar);
  playbackMenu(menuBar);
  zoomViewMenu(menuBar);
  helpMenu(menuBar);
  layout->setMenuBar(menuBar);

  statusLabel = new QLabel("No file loaded. Open a video to begin.");
  statusLabel->setStyleSheet(
      "background-color: #222; color: #aaa; padding: 5px; font-style: italic;");
  layout->addWidget(statusLabel);

  videoSurface = new VideoSurface(this);
  videoSurface->setStyleSheet("background-color: black;");
  videoSurface->setMinimumHeight(300);
  videoSurface->setMouseTracking(true);
  layout->addWidget(videoSurface, 1);

  // timecode always visible — frame reference must survive immersive mode
  timecodeLabel = new QLabel("00:00:00:00");
  timecodeLabel->setAlignment(Qt::AlignCenter);
  timecodeLabel->setStyleSheet("font-family: monospace; font-size: 24pt; "
                               "color: white; background-color: #111;");
  layout->addWidget(timecodeLabel);

  // toolbar: groups controls that hide in immersive mode
  toolbar = new QWidget(this);
  QVBoxLayout *tbLayout = new QVBoxLayout(toolbar);
  tbLayout->setContentsMargins(0, 0, 0, 0);
  tbLayout->setSpacing(0);

  audioBox = new QComboBox(toolbar);
  audioBox->addItem("No file loaded");
  audioBox->setStyleSheet(
      "background-color: #222; color: white; padding: 4px;");
  tbLayout->addWidget(audioBox);

  hwAccelBox = new QCheckBox("Hardware Acceleration (auto-detect)", toolbar);
  hwAccelBox->setChecked(true);
  hwAccelBox->setStyleSheet(
      "color: white; background-color: #111; padding: 4px;");
  tbLayout->addWidget(hwAccelBox);

  layout->addWidget(toolbar);

  // audio stream switching
  audioBox->blockSignals(false);
  connect(audioBox, &QComboBox::currentIndexChanged, [this](int index) {
    if (!audioBox->isEnabled())
      return;
    if (!player.uri)
      return;
    player.selected_audio_stream = audioBox->itemData(index).toInt();
    gint64 saved_pos = 0;
    if (player.pipeline)
      gst_element_query_position(player.pipeline, GST_FORMAT_TIME, &saved_pos);
    load_new_video(QString(player.uri));
    QTimer::singleShot(400, [this, saved_pos]() {
      if (saved_pos > 0 && player.pipeline)
        gst_element_seek_simple(
            player.pipeline, GST_FORMAT_TIME,
            (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
            saved_pos);
    });
  });

  connect(hwAccelBox, &QCheckBox::toggled, [this](bool checked) {
    player.use_hw_accel = checked ? 1 : 0;
    if (player.uri)
      load_new_video(QString(player.uri));
  });

  // auto-hide timer for immersive mode
  hideTimer = new QTimer(this);
  hideTimer->setSingleShot(true);
  connect(hideTimer, &QTimer::timeout, [this]() {
    if (isAutohideActive)
      hideUI();
  });

  // timecode update timer
  QTimer *tcTimer = new QTimer(this);
  connect(tcTimer, &QTimer::timeout, [this]() {
    if (player.pipeline && player.assignedFPS > 0.0) {
      char *tc = update_live_timecode(&player);
      QString newTc = QString(tc);
      if (tc && newTc != "00:00:00:00") {
        timecodeLabel->setText(newTc);
      } else if (newTc == "00:00:00:00") {
        gint64 pos;
        if (gst_element_query_position(player.pipeline, GST_FORMAT_TIME,
                                       &pos) &&
            pos == 0)
          timecodeLabel->setText(newTc);
      }
      if (tc)
        g_free(tc);
    }
  });
  tcTimer->start(30);

  player.window_id = (unsigned long)videoSurface->winId();
  init_idle_pipeline(&player);

  QTimer::singleShot(100, [this]() {
    set_video_window(&player, player.window_id);
    gst_element_set_state(player.pipeline, GST_STATE_PLAYING);
  });

  if (argc > 1) {
    QTimer::singleShot(250,
                       [this, argv]() { load_new_video(QString(argv[1])); });
  }
}
void VideoWindow::init_screen() {
  isFullscreenActive = false;
  isAutohideActive = false;
  hideTimer = nullptr;
}

void VideoWindow::mainFileMenu(QMenuBar *menuBar) {
  QMenu *fileMenu = menuBar->addMenu(tr("&File"));

  QAction *openAction = fileMenu->addAction(tr("&Open Video..."));
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this,
          &VideoWindow::open_file_dialog);

  closeVideoAction = fileMenu->addAction(tr("&Close Video"));
  closeVideoAction->setShortcut(QKeySequence::Close);
  closeVideoAction->setEnabled(false);
  connect(closeVideoAction, &QAction::triggered, this,
          &VideoWindow::close_recent_video);

  QAction *quitAction = fileMenu->addAction(tr("&Quit"));
  quitAction->setShortcut(QKeySequence::Quit);
  connect(quitAction, &QAction::triggered, this, &QWidget::close);
}

//zoom view menu, for mouse users
void VideoWindow::zoomViewMenu(QMenuBar *menuBar) {
  QMenu *zoomViewMenu = menuBar->addMenu(tr("&View"));

  QAction *toggleFullAction = zoomViewMenu->addAction(tr("&Toggle Fullscreen"));
  toggleFullAction->setShortcut(QKeySequence(Qt::Key_F));
  connect(toggleFullAction, &QAction::triggered,
        [this]() {  this->toggleFullscreen();});

  QAction *hideMenuAction = zoomViewMenu->addAction(tr("&Autohide Menu"));
  hideMenuAction->setShortcut(QKeySequence(Qt::Key_F11));
  connect(hideMenuAction, &QAction::triggered,
        [this]() {  this->toggleAutohide();});

  zoomViewMenu->addSeparator();

  QAction *scrollUpAction = zoomViewMenu->addAction(tr("&Scroll Up"));
  scrollUpAction->setShortcut(QKeySequence(Qt::Key_W));
  connect(scrollUpAction, &QAction::triggered,
        [this]() {  zoom_video(&this->player, "UP");});

  QAction *scrollDownAction = zoomViewMenu->addAction(tr("&Scroll Down"));
  scrollDownAction->setShortcut(QKeySequence(Qt::Key_S));
  connect(scrollDownAction, &QAction::triggered,
        [this]() {  zoom_video(&this->player, "DOWN");});

  zoomViewMenu->addSeparator();

  QAction *scrollLeftAction = zoomViewMenu->addAction(tr("&Scroll Left"));
  scrollLeftAction->setShortcut(QKeySequence(Qt::Key_A));
  connect(scrollLeftAction, &QAction::triggered,
        [this]() {  zoom_video(&this->player, "LEFT");});

  QAction *scrollRightAction = zoomViewMenu->addAction(tr("&Scroll Right"));
  scrollRightAction->setShortcut(QKeySequence(Qt::Key_D));
  connect(scrollRightAction, &QAction::triggered,
        [this]() {  zoom_video(&this->player, "RIGHT");});

  zoomViewMenu->addSeparator();

  QAction *shrinkScaleAction = zoomViewMenu->addAction(tr("&Shrink Scale"));
  shrinkScaleAction->setShortcut(QKeySequence(Qt::Key_I));
  connect(shrinkScaleAction, &QAction::triggered,
        [this]() {  zoom_video(&this->player, "SHRINK");});

  QAction *resetScaleAction = zoomViewMenu->addAction(tr("&Reset Scale"));
  resetScaleAction->setShortcut(QKeySequence(Qt::Key_O));
  connect(resetScaleAction, &QAction::triggered,
        [this]() {  zoom_video(&this->player, "RESET");});

  QAction *enlargeScaleAction = zoomViewMenu->addAction(tr("&Enlarge Scale"));
  enlargeScaleAction->setShortcut(QKeySequence(Qt::Key_P));
  connect(enlargeScaleAction, &QAction::triggered,
        [this]() {  zoom_video(&this->player, "ENLARGE");});

  zoomViewMenu->addSeparator();

}

void VideoWindow::helpMenu(QMenuBar *menuBar) {
  QMenu *helpMenu = menuBar->addMenu(tr("&Help"));

  QAction *aboutAction = helpMenu->addAction(tr("&About..."));
  connect(aboutAction, &QAction::triggered, [this]() {
    QMessageBox::about(
        this, tr("About Mini Timeline View"),
        tr("<h3>Mini Timeline View</h3>"
           "<p>A basic keyboard controlled media player.</p>"
           "<p>© 2026 jpongsin</p>"
           "<p>Licensed under MIT License</p>"
           "<p>For more info, refer to README.md and LICENSE.md</p>"));
  });
  helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}

void VideoWindow::playbackMenu(QMenuBar *menuBar) {
  QMenu *playMenu = menuBar->addMenu(tr("&Playback"));

  QAction *playAction = playMenu->addAction(tr("Play/Pause"));
  playAction->setShortcut(QKeySequence(Qt::Key_Space));
  connect(playAction, &QAction::triggered,
          [this]() { toggle_playback(&player); });
  playMenu->addSeparator();

  QAction *fwdAction = playMenu->addAction(tr("Step Forward (10 frames)"));
  fwdAction->setShortcut(QKeySequence(Qt::Key_L));
  connect(fwdAction, &QAction::triggered,
          [this]() { seek_mechanism(&player, 10); });

  QAction *backAction = playMenu->addAction(tr("Step Backward (10 frames)"));
  backAction->setShortcut(QKeySequence(Qt::Key_J));
  connect(backAction, &QAction::triggered,
          [this]() { seek_mechanism(&player, -10); });
  playMenu->addSeparator();

  QAction *spUpAction = playMenu->addAction(tr("Increase Speed (+0.25)"));
  spUpAction->setShortcut(QKeySequence(Qt::Key_BracketRight));
  connect(spUpAction, &QAction::triggered,
          [this]() { change_rate(&player, 0.25); });

  QAction *spDnAction = playMenu->addAction(tr("Decrease Speed (-0.25)"));
  spDnAction->setShortcut(QKeySequence(Qt::Key_BracketLeft));
  connect(spDnAction, &QAction::triggered,
          [this]() { change_rate(&player, -0.25); });
  playMenu->addSeparator();

  QAction *volUpAction = playMenu->addAction(tr("Increase Volume (+0.10)"));
  volUpAction->setShortcut(QKeySequence(Qt::Key_Up));
  connect(volUpAction, &QAction::triggered,
          [this]() { change_volume(&player, 0.10); });

  QAction *volDnAction = playMenu->addAction(tr("Decrease Volume (-0.10)"));
  volDnAction->setShortcut(QKeySequence(Qt::Key_Down));
  connect(volDnAction, &QAction::triggered,
          [this]() { change_volume(&player, -0.10); });
  playMenu->addSeparator();

  QAction *homeAction = playMenu->addAction(tr("Go to Start"));
  homeAction->setShortcut(QKeySequence(Qt::Key_Home));
  connect(homeAction, &QAction::triggered,
          [this]() { seek_begin(&player, 0); });

  QAction *ssAction = playMenu->addAction(tr("&Save Screenshot..."));
  ssAction->setShortcut(QKeySequence(Qt::Key_B));
  connect(ssAction, &QAction::triggered, this, &VideoWindow::export_screenshot);
}

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

void VideoWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  refreshVideoRect();
}

bool VideoWindow::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if (handle_hotkeys(keyEvent, &player, this))
      return true;
  }
  return QWidget::eventFilter(obj, event);
}

bool handle_hotkeys(QKeyEvent *event, VideoPlayer *player,
                    VideoWindow *window) {
  if (QApplication::activeModalWidget() != nullptr)
    return false;

  switch (event->key()) {
    //zoom video functionality
    case Qt::Key_W:
      zoom_video(&window->player, "UP");
      return true;
    case Qt::Key_A:
      zoom_video(&window->player, "LEFT");
      return true;
    case Qt::Key_S:
      zoom_video(&window->player, "DOWN");
      return true;
    case Qt::Key_D:
      zoom_video(&window->player, "RIGHT");
      return true;
    case Qt::Key_I:
      zoom_video(&window->player, "SHRINK");
      return true;
    case Qt::Key_O:
      zoom_video(&window->player, "RESET");
      return true;
    case Qt::Key_P:
      zoom_video(&window->player, "ENLARGE");
      return true;
      //scrob by 10 frames
    case Qt::Key_J:
      seek_mechanism(player, -10);
      return true;
    case Qt::Key_L:
      seek_mechanism(player, 10);
      return true;
      //play-pause
    case Qt::Key_K:
    case Qt::Key_Space:
      toggle_playback(player);
      return true;
      //screenshot
    case Qt::Key_B:
      window->export_screenshot();
      return true;
      //single frame scrobbing
    case Qt::Key_Left:
      on_key_left(player);
      return true;
    case Qt::Key_Right:
      on_key_right(player);
      return true;
      //windowed fullscreen
    case Qt::Key_F:
      window->toggleFullscreen();
      return true;
      //autohide
    case Qt::Key_F11:
      window->toggleAutohide();
      return true;
      //rewind to beginning
    case Qt::Key_Home:
      seek_begin(player, 0);
      return true;
      //mute
    case Qt::Key_M:
      toggle_mute(player);
      return true;
      //speed
    case Qt::Key_BracketRight:
      change_rate(player, 0.25);
      return true;
    case Qt::Key_BracketLeft:
      change_rate(player, -0.25);
      return true;
      //volume
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