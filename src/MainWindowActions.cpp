// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/MainWindow.h"
#include <QActionGroup>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QShortcut>
#include <QStatusBar>
#include <algorithm>

void MainWindow::openVideo() {
  //check for known formats
  QString fileName = QFileDialog::getOpenFileName(
      this, "Open Video", "",
      "Video Files (*.mp4 *.mkv "
      "*.mov *.avi "
      "*.webm *.flv "
      "*.vob *.ogv "
      "*.ogg *.wmv "
      "*.mpg *.mpeg "
      "*.m4v *.3gp "
      "*.3g2 *.mxf "
      "*.f4v)");

  //confirm loading video...
  if (!fileName.isEmpty()) {
    m_playbackMenu->setEnabled(true);
    updateWindowShortcuts();
    mpv_set_property_string(m_mpv, "video-aspect-override", "-1");
    m_lastVideoDir = QFileInfo(fileName).absolutePath();
    m_videoWidget->loadFile(fileName);
    m_audioMenu->clear();
    m_audioMenu->setEnabled(true);
    m_fpsLabel->setText("FPS: N/A"); // Reset until metadata probed
  }
}

void MainWindow::openVideoCLI(const QString &fileName) {
  if (fileName.isEmpty()) return;

  QByteArray utf8 = fileName.toUtf8();
  const char *cmd[] = {"loadfile", utf8.constData(), "replace", NULL};
  mpv_command(m_mpv, cmd);

  m_playbackMenu->setEnabled(true);
  updateWindowShortcuts();
  mpv_set_property_string(m_mpv, "video-aspect-override", "-1");
  m_lastVideoDir = QFileInfo(fileName).absolutePath();
  m_videoWidget->loadFile(fileName);
  m_audioMenu->clear();
  m_audioMenu->setEnabled(true);
  m_fpsLabel->setText("FPS: N/A"); // Reset until metadata probed
}

void MainWindow::populateAudioMenu(const VideoState &vs) {
  //clear audio
  m_audioMenu->clear();

  //populate audio track
  for (int i = 0; i < vs.nb_audio_tracks; i++) {
    QString label = QString("Track %1 [%2] %3")
                        .arg(vs.audio_tracks[i].index)
                        .arg(vs.audio_tracks[i].language)
                        .arg(vs.audio_tracks[i].title);
    //make sure the audio menu corresponds
    QAction *action = m_audioMenu->addAction(label);
    action->setCheckable(true);
    m_audioActionGroup->addAction(action);
    int id = vs.audio_tracks[i].index;

    //reinforce
    connect(action, &QAction::triggered, this,
            [this, id]() { set_audio_track(m_mpv, id); });
  }
}

void MainWindow::closeVideo() {
  //if video already closed, dont do anything
  if (!m_mpv)
    return;

  //reset video and view
  m_videoWidget->resetVideo();
  m_videoWidget->resetView();
  m_playbackMenu->setEnabled(false);
  updateWindowShortcuts();

  //reset audio
  //need to depopulate it
  m_audioMenu->clear();
  m_audioMenu->setEnabled(false);

  //setup idle video again
  m_duration = 0;
  if (m_fps <= 0)
    m_fps = 30.0;

  //performance tweaking
  mpv_set_property_string(m_mpv, "video-aspect-override", "16:9");
  mpv_set_option_string(m_mpv, "pause", "yes");
  load_file(m_mpv, "av://lavfi:smptehdbars");
}

//mpv arguments
void MainWindow::changeSpeed(double delta) {
  double current = 1.0;
  mpv_get_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &current);
  double next = std::clamp(current + delta, 0.25, 2.0);
  mpv_set_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &next);
}

void MainWindow::changeVolume(double delta) {
  double current = 100.0;
  mpv_get_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &current);
  double next = std::clamp(current + delta, 0.0, 100.0);
  mpv_set_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &next);
}

void MainWindow::toggleMute() {
  int mute = 0;
  mpv_get_property(m_mpv, "mute", MPV_FORMAT_FLAG, &mute);
  mute = !mute;
  mpv_set_property(m_mpv, "mute", MPV_FORMAT_FLAG, &mute);
}

//mpv commands
void MainWindow::togglePlayPause() { toggle_pause(m_mpv); }
void MainWindow::seekForward1() { mpv_command_string(m_mpv, "frame-step"); }
void MainWindow::seekBackward1() {mpv_command_string(m_mpv, "frame-back-step");}
void MainWindow::seekForward5() {mpv_command_string(m_mpv, "seek 5 relative exact");}
void MainWindow::seekBackward5() {mpv_command_string(m_mpv, "seek -5 relative exact");}
void MainWindow::resetToBeginning() { seek_absolute(m_mpv, 0, 1); }
void MainWindow::toggleFullscreen() {if (isFullScreen()) {showNormal();} else{ showFullScreen();}}

void MainWindow::assignScreenshotFolder() {
  //get available directory
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Screenshot Folder", m_screenshotFolder);
  //if directory exists
  if (!dir.isEmpty()) {m_screenshotFolder = dir;}
}

//change bool on a private variable
void MainWindow::toggleUninterruptedSnap(bool checked) {m_uninterruptedSnap = checked;}

void MainWindow::takeScreenshot() {
  //taking screenshots on an idle video
  //disable.
  if (!m_mpv || m_isIdle)
    return;

  // Pause if playing for a clean capture
  int paused = 0;
  mpv_get_property(m_mpv, "pause", MPV_FORMAT_FLAG, &paused);

  //not paused; start pause.
  if (!paused) {
    int p = 1;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &p);
  }

  //screenshot folder will be usually the same as imported video's directory
  QString defaultDir = m_screenshotFolder;

  //directory will revert to home if not the case.
  if (defaultDir.isEmpty()) {
    defaultDir = m_lastVideoDir.isEmpty() ? QDir::homePath() : m_lastVideoDir;
  }

  //distract-free snapping
  if (m_uninterruptedSnap) {
    // auto-generate filename
    QString timestamp =
        QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString filename = QString("Screenshot_%1.png").arg(timestamp);
    QString fullPath = QDir(defaultDir).absoluteFilePath(filename);

    QString cmd = QString("screenshot-to-file \"%1\" video").arg(fullPath);
    mpv_command_string(m_mpv, cmd.toUtf8().constData());
    statusBar()->showMessage("Screenshot saved to: " + filename, 3000);
  } else {
    // prompt mode
    QString filter = "PNG Image (*.png);;JPEG Image (*.jpg);;WebP Image "
                     "(*.webp);;TIFF Image (*.tif)";
    QString selectedFilter;
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save Screenshot", defaultDir, filter, &selectedFilter);

    if (!filePath.isEmpty()) {
      QString cmd = QString("screenshot-to-file \"%1\" video").arg(filePath);
      mpv_command_string(m_mpv, cmd.toUtf8().constData());
    }
  }
}