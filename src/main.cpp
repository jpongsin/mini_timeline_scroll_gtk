//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info

#include "../include/video_window.h"

#ifdef __linux__

#include <X11/Xlib.h>
#undef KeyPress
#undef KeyRelease

#endif

//explicitly prioritize nvidia if there are any
void boost_nvidia_ranks() {
    GstRegistry *registry = gst_registry_get();
    //check nvidia hardware decoders
    const gchar *nv_decoders[] = { "nvh264dec", "nvh265dec", "nvvp9dec", "nvjpegdec" };

    for (int i = 0; i < 4; i++) {
        GstElementFactory *factory = gst_element_factory_find(nv_decoders[i]);
        if (factory) {
            // Set rank higher than the default software rank (usually 256)
            gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(factory), GST_RANK_PRIMARY + 100);
            gst_object_unref(factory);
            g_print("loaded: %s\n", nv_decoders[i]);
        }
    }
}
//wrapper for app to reinforce hotkeys
class HotkeyApp : public QApplication {
public:
    //constructor
    HotkeyApp(int &argc, char **argv) : QApplication(argc, argv), videoWin(nullptr) {
    }

    VideoWindow *videoWin;

    //if a hotkey was pressed while videowin is active
    //ensure the keys are accepted
    bool notify(QObject *receiver, QEvent *event) override {
        if (event->type() == QEvent::KeyPress && videoWin) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (handle_hotkeys(keyEvent, &videoWin->player, videoWin)) {
                event->accept();
                return true;
            }
        }
        return QApplication::notify(receiver, event);
    }
};

//main program
int main(int argc, char *argv[]) {
#ifdef __linux__
  qputenv("QT_QPA_PLATFORM", "xcb");
  qputenv("GST_GL_WINDOW", "x11");
  qunsetenv("WAYLAND_DISPLAY");
#endif
    gst_init(&argc, &argv);
#ifdef __NVIDIA__
    boost_nvidia_ranks();
#endif
#ifdef __linux__
    XInitThreads();
#endif
    HotkeyApp a(argc, argv);

    //  set color of app
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    a.setPalette(darkPalette);
    // ---------------------------

    VideoWindow window(argc, argv);
    a.videoWin = &window;

    window.show();
    return a.exec();
}
