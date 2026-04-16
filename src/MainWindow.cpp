// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/MainWindow.h"
#include <QActionGroup>
#include <QApplication>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStatusBar>
#include <QVBoxLayout>
#include <cmath>
#include <functional>

MainWindow::MainWindow(int argc, char* argv[], QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Mini Timeline Scroll");
    resize(1280, 800);

    //initialize mpv screen
    //and videowidget
    m_mpv = init_backend();
    m_videoWidget = new VideoWidget(this);
    m_videoWidget->setMpvHandle(m_mpv);

    //create menu
    m_accelActionGroup = new QActionGroup(this);
    createMenus();
    m_audioActionGroup = new QActionGroup(this);


    //create overlay
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    //create bottom bars
    createInfoBar();
    mainLayout->addWidget(m_videoWidget, 1);
    createToolbar();

    //setup remaining shortcuts
    setupShortcuts();

    //check if mpv is installed
    if (m_mpv) {
        // initial state loads SMPTE bars
        QTimer::singleShot(0, this, [this, argc, argv]() {
            mpv_set_property_string(m_mpv, "video-aspect-override", "16:9");
            m_seekSlider->setEnabled(false);
            load_file(m_mpv, "av://lavfi:smptehdbars");
            if (argc > 1) {
                QTimer::singleShot(250,
                       [this, argv]() {
                           openVideoCLI(QString::fromUtf8(argv[1]));
                       });
}
        });

        //update the UI
        m_uiTimer = new QTimer(this);
        connect(m_uiTimer, &QTimer::timeout, this, &MainWindow::updateUI);
        m_uiTimer->start(33);

    } else {
        //error message.
        QMessageBox::critical(this, "Fatal Error",
                              "Failed to initialize libmpv backend. Check your "
                              "installation and audio/video drivers.");
    }
}

// cleanup the rendering and properly closing the window.
MainWindow::~MainWindow() {
    delete m_videoWidget;
    cleanup_backend(m_mpv);
}

void MainWindow::createMenus() {
    //file menu part 1
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open Video", QKeySequence::Open, this,
                        &MainWindow::openVideo);
    m_closeAction = fileMenu->addAction("&Close Video", QKeySequence::Close, this,
                                        &MainWindow::closeVideo);

    fileMenu->addSeparator();

    //file menu part 2
    m_screenshotAction =
            fileMenu->addAction("Screenshot", QKeySequence(Qt::Key_B), this,
                                [this]() { takeScreenshot(); });

    m_assignSnapFolderAction = fileMenu->addAction(
        "Assign Screenshot Folder", this, &MainWindow::assignScreenshotFolder);

    QAction *snapToggle = fileMenu->addAction(
        "Uninterrupted Screenshot", this, &MainWindow::toggleUninterruptedSnap);
    snapToggle->setCheckable(true);
    snapToggle->setChecked(m_uninterruptedSnap);

    fileMenu->addSeparator();
    fileMenu->addAction("&Exit", QKeySequence::Quit, qApp, &QApplication::quit);

    //playback menu
    m_playbackMenu = menuBar()->addMenu("&Playback");
    QAction *playPauseAction = m_playbackMenu->addAction(
        "&Play/Pause", this, &MainWindow::togglePlayPause);
    playPauseAction->setShortcuts(
        {QKeySequence(Qt::Key_Space), QKeySequence(Qt::Key_K)});

    QAction *seekForwardAction =
            m_playbackMenu->addAction("Seek +1 Frame", this, &MainWindow::seekForward1);
    seekForwardAction->setShortcuts(
        {QKeySequence(Qt::Key_Right), QKeySequence(Qt::Key_L)});

    QAction *seekBackwardAction = m_playbackMenu->addAction(
        "Seek -1 Frame", this, &MainWindow::seekBackward1);
    seekBackwardAction->setShortcuts(
        {QKeySequence(Qt::Key_Left), QKeySequence(Qt::Key_J)});

    QAction *seekForward5Action = m_playbackMenu->addAction(
        "Seek +5 Seconds", this, &MainWindow::seekForward5);
    seekForward5Action->setShortcuts({
        QKeySequence(Qt::SHIFT | Qt::Key_L),
        QKeySequence(Qt::SHIFT | Qt::Key_Right)
    });

    QAction *seekBackward5Action = m_playbackMenu->addAction(
        "Seek -5 Seconds", this, &MainWindow::seekBackward5);
    seekBackward5Action->setShortcuts({
        QKeySequence(Qt::SHIFT | Qt::Key_J),
        QKeySequence(Qt::SHIFT | Qt::Key_Left)
    });

    m_playbackMenu->addAction("Go to Beginning", QKeySequence(Qt::Key_Home), this,
                              &MainWindow::resetToBeginning);
    m_playbackMenu->addSeparator();
    m_playbackMenu->addAction("Decrease Speed", (QKeySequence(Qt::Key_BracketLeft)),
                              this, [this]() { changeSpeed(-0.25); });
    m_playbackMenu->addAction("Increase Speed",
                              (QKeySequence(Qt::Key_BracketRight)), this,
                              [this]() { changeSpeed(0.25); });

    m_playbackMenu->addSeparator();

    m_playbackMenu->addAction("Decrease Volume", (QKeySequence(Qt::Key_Down)), this,
                              [this]() { changeVolume(-5.0); });
    m_playbackMenu->addAction("Increase Volume", (QKeySequence(Qt::Key_Up)), this,
                              [this]() { changeVolume(5.0); });
    m_playbackMenu->addAction("Mute Volume", (QKeySequence(Qt::Key_M)), this,
                              [this]() { toggleMute(); });

    m_playbackMenu->setEnabled(false);


    //audio menu
    m_audioMenu = menuBar()->addMenu("&Audio");
    m_audioMenu->setEnabled(false);

    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Zoom In", Qt::Key_P, m_videoWidget,
                        &VideoWidget::zoomPlus);
    viewMenu->addAction("Zoom Out", Qt::Key_O, m_videoWidget,
                        &VideoWidget::zoomMinus);
    viewMenu->addAction("Navigate Left", Qt::Key_A, m_videoWidget,
                        &VideoWidget::moveLeft);
    viewMenu->addAction("Navigate Right", Qt::Key_D, m_videoWidget,
                        &VideoWidget::moveRight);
    viewMenu->addAction("Navigate Up", Qt::Key_W, m_videoWidget,
                        &VideoWidget::moveUp);
    viewMenu->addAction("Navigate Down", Qt::Key_S, m_videoWidget,
                        &VideoWidget::moveDown);
    viewMenu->addAction("Reset Scale", Qt::Key_I, m_videoWidget,
                        &VideoWidget::resetView);

    //acceleration menu
    m_accelMenu = menuBar()->addMenu("&Acceleration");
    m_accelAuto = m_accelMenu->addAction("Autodiscover");
    m_accelHardware = m_accelMenu->addAction("[Hardware Accelerated]");
    m_accelSoftware = m_accelMenu->addAction("[Software Rendering]");

    m_accelAuto->setCheckable(true);
    m_accelHardware->setCheckable(true);
    m_accelSoftware->setCheckable(true);

    m_accelActionGroup->addAction(m_accelAuto);
    m_accelActionGroup->addAction(m_accelHardware);
    m_accelActionGroup->addAction(m_accelSoftware);

    connect(m_accelAuto, &QAction::triggered, this, [this]() {
        mpv_set_property_string(m_mpv, "hwdec", "auto");
    });
    connect(m_accelHardware, &QAction::triggered, this, [this]() {
        mpv_set_property_string(m_mpv, "hwdec", "auto-copy");
    });
    connect(m_accelSoftware, &QAction::triggered, this, [this]() {
        mpv_set_property_string(m_mpv, "hwdec", "no");
    });

    //help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAction = helpMenu->addAction(tr("&About..."));
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr("About Mini Timeline Scroll"),
            tr("<h3>Mini Timeline Scroll</h3>"
                "<p>A basic keyboard controlled media player.</p>"
                "<p>© 2026 jpongsin</p>"
                "<p>Licensed under MIT License</p>"
                "<p>For more info, refer to README.md and LICENSE.md</p>"));
    });
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);


    //add fullscreen and toggle UI to stabilize hiding UI.
    QAction *fsAction =
            viewMenu->addAction("Fullscreen", this, &MainWindow::toggleFullscreen);
    fsAction->setShortcuts({QKeySequence(Qt::Key_F11), QKeySequence(Qt::Key_F)});

    QAction *hideUIAction = viewMenu->addAction("Toggle UI", this, [this]() {
        bool visible = !m_bottomToolbar->isVisible();
        menuBar()->setVisible(visible);
        m_infoBar->setVisible(visible);
        m_bottomToolbar->setVisible(visible);
    });
    hideUIAction->setShortcut(QKeySequence(Qt::Key_H));

    //update window shortcuts
    updateWindowShortcuts();
}

void MainWindow::updateWindowShortcuts() {
    // Sync the window's top-level actions with the current menu states
    for (auto *menu: menuBar()->findChildren<QMenu *>()) {
        for (auto *action: menu->actions()) {
            //if video is idle and hotkeys are disabled, carry over
            if (menu->isEnabled()) this->addAction(action);
            else this->removeAction(action);
        }
    }
}


void MainWindow::createInfoBar() {
    //set information
    m_infoBar = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(m_infoBar);
    m_filenameLabel = new QLabel(" ", this);
    m_fpsLabel = new QLabel("FPS: N/A", this);
    m_speedLabel = new QLabel("1.00x", this);
    m_volumeLabel = new QLabel("VOL: 100%", this);

    //add widget to layout
    layout->addWidget(m_filenameLabel);
    layout->addStretch();
    layout->addWidget(m_speedLabel);
    layout->addWidget(m_volumeLabel);
    layout->addWidget(m_fpsLabel);
    centralWidget()->layout()->addWidget(m_infoBar);
}

void MainWindow::createToolbar() {
    //create slider and toolbar
    //timecode
    m_bottomToolbar = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(m_bottomToolbar);
    m_timecodeLabel = new QLabel("00:00:00:00", this);
    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 10000);

    //slider
    connect(m_seekSlider, &QSlider::sliderMoved, this, [this](int val) {
        double pos = (double) val / 10000.0 * m_duration;
        seek_absolute(m_mpv, pos, 0);
    });

    layout->addWidget(m_timecodeLabel);
    layout->addWidget(m_seekSlider, 1);
    centralWidget()->layout()->addWidget(m_bottomToolbar);
}

void MainWindow::setupShortcuts() {
    //esc to exit fullscreen and restore UI
    QShortcut *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (isFullScreen())
            showNormal();
        menuBar()->setVisible(true);
        m_infoBar->setVisible(true);
        m_bottomToolbar->setVisible(true);
    });
}

//calculate timecode
QString MainWindow::formatTimecode(double seconds, double fps) {
    //guarding
    if (fps <= 0)
        return "00:00:00:00";

    std::function<bool(double, double)> near = [](double a, double b) {
        return std::abs(a - b) < 0.001;
    };

    //timecode
    bool isNTSC = near(fps, 30000.0 / 1001.0);
    bool isNTSCDouble = near(fps, 60000.0 / 1001.0);
    bool dropFrame = isNTSC || isNTSCDouble;

    int nominalFps = (int)std::round(fps); // 30 or 60

    long long totalFrames = (long long)std::floor(seconds * fps + 0.0001);

    //dropframe calculation
    if (dropFrame) {
        int dropFrames = isNTSCDouble ? 4 : 2;
        //int framesPerHour = nominalFps * 60 * 60;
        int framesPer10Minutes = nominalFps * 60 * 10;
        int framesPerMinute = nominalFps * 60;

        long long d = totalFrames / (framesPer10Minutes - dropFrames * 9);
        long long m = totalFrames % (framesPer10Minutes - dropFrames * 9);

        long long dropped =
            dropFrames * 9 * d +
            dropFrames * ((m - dropFrames) / (framesPerMinute - dropFrames));

        if (m < dropFrames)
            dropped = dropFrames * 9 * d;

        totalFrames += dropped;
    }

    //formatting and calculating
    int f = totalFrames % nominalFps;
    int s = (totalFrames / nominalFps) % 60;
    int m = (totalFrames / (nominalFps * 60)) % 60;
    int h = totalFrames / (nominalFps * 3600);
    char sep = dropFrame ? ';' : ':';

    //show timecode to UI
    return QString("%1:%2:%3%4%5")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(sep)
        .arg(f, 2, 10, QChar('0'));
}

void MainWindow::updateUI() {
    //guarding if no stream available.
    if (!m_mpv)
        return;

    //check if it's idling, then free path
    char *path = mpv_get_property_string(m_mpv, "path");
    m_isIdle = (path && QString(path).startsWith("av://lavfi:"));
    bool isIdle = m_isIdle;
    mpv_free(path);

    //define time position and duration
    double pos = 0;
    mpv_get_property(m_mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos);
    mpv_get_property(m_mpv, "duration", MPV_FORMAT_DOUBLE, &m_duration);

    //enable playbacks, timecode, sliders
    //else, idle video=SMPTE, disable playbacks
    if (!isIdle && m_duration > 0 && !std::isinf(m_duration)) {
        m_seekSlider->setEnabled(true);
        m_closeAction->setEnabled(true);
        m_screenshotAction->setEnabled(true);
        m_assignSnapFolderAction->setEnabled(true);
        m_seekSlider->setValue((int) (pos / m_duration * 10000.0));
        m_timecodeLabel->setText(formatTimecode(pos, m_fps));
    } else {
        m_seekSlider->setEnabled(false);
        m_closeAction->setEnabled(false);
        m_screenshotAction->setEnabled(false);
        m_assignSnapFolderAction->setEnabled(false);
        m_seekSlider->setValue(0);
        m_timecodeLabel->setText("00:00:00:00");
    }

    //name of video
    VideoState vs = m_videoWidget->getMetadata();
    m_filenameLabel->setText(isIdle ? "No file loaded. Open a video to begin." : vs.filename);
    m_filenameLabel->setStyleSheet(
        "background-color: #222; color: #aaa; padding: 5px; font-style: italic;");

    //fps
    m_fps = vs.fps;
    m_fpsLabel->setText(
        isIdle
            ? " "
            : QString("FPS: %1 | %2").arg(m_fps).arg(vs.codec_name));

    //hardware acceleration state
    char *hwdec = mpv_get_property_string(m_mpv, "hwdec");
    if (hwdec) {
        QString hw(hwdec);
        if (hw == "no") m_accelSoftware->setChecked(true);
        else if (hw == "auto" || hw == "auto-safe") m_accelAuto->setChecked(true);
        else m_accelHardware->setChecked(true);
        mpv_free(hwdec);
    }

    // Speed & Volume status
    double speed = 1.0, volume = 100.0;
    int mute = 0;
    mpv_get_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &speed);
    mpv_get_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
    mpv_get_property(m_mpv, "mute", MPV_FORMAT_FLAG, &mute);

    m_speedLabel->setText(QString("%1x").arg(speed, 0, 'f', 2));
    m_volumeLabel->setText(
        QString("VOL: %1% %2").arg((int) volume).arg(mute ? "[MUTE]" : ""));

    if (!isIdle && vs.nb_audio_tracks > 0 && m_audioMenu->isEmpty()) {
        populateAudioMenu(vs);
    }
}
