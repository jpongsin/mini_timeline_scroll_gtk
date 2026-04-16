// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "VideoWidget.h"
#include <QCheckBox>
#include <QLabel>
#include <QMainWindow>
#include <QSlider>
#include <QTimer>

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private slots:
  void openVideo();
  void closeVideo();
  void updateUI();
  void updateWindowShortcuts();

  // Playback
  void togglePlayPause();
  void seekForward1();
  void seekBackward1();
  void seekForward5();
  void seekBackward5();
  void resetToBeginning();
  void changeSpeed(double delta);
  void changeVolume(double delta);
  void toggleMute();

  // View
  void takeScreenshot();
  void assignScreenshotFolder();
  void toggleUninterruptedSnap(bool checked);
  void toggleFullscreen();

private:
  void createMenus();
  void createInfoBar();
  void createToolbar();
  void setupShortcuts();
  void populateAudioMenu(const VideoState &vs);
  QString formatTimecode(double seconds, double fps);

  VideoWidget *m_videoWidget;
  QMenu *m_audioMenu{};
  QMenu *m_accelMenu{};
  QMenu *m_playbackMenu{};
  QActionGroup *m_audioActionGroup;
  QActionGroup *m_accelActionGroup;
  QAction *m_closeAction{};
  QAction *m_screenshotAction{};
  QAction *m_assignSnapFolderAction{};
  QAction *m_accelAuto{};
  QAction *m_accelHardware{};
  QAction *m_accelSoftware{};

  // Screenshot state
  QString m_screenshotFolder;
  QString m_lastVideoDir;
  bool m_uninterruptedSnap = false;
  bool m_isIdle = true;

  // Info Bar
  QWidget *m_infoBar{};
  QLabel *m_filenameLabel{};
  QLabel *m_fpsLabel{};
  QLabel *m_speedLabel{};
  QLabel *m_volumeLabel{};

  // Toolbar
  QWidget *m_bottomToolbar{};
  QLabel *m_timecodeLabel{};
  QSlider *m_seekSlider{};

  QTimer *m_uiTimer;
  mpv_handle *m_mpv;
  double m_duration = 0;
  double m_fps = 30;
};

#endif // MAINWINDOW_H
