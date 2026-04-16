// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#ifndef MLT_CALL_API_VIDEOWIDGET_H
#define MLT_CALL_API_VIDEOWIDGET_H

#include "video_backend.h"
#include <QOpenGLFunctions>
#include <QtOpenGLWidgets/QOpenGLWidget>

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    ~VideoWidget() override;

    // mpv interaction
    void setMpvHandle(mpv_handle *handle);

    // Metadata access
    VideoState getMetadata();

public slots:
    void loadFile(const QString &path);

    void resetVideo();

    // Zoom/Pan (mapped to mpv properties)
    void zoomPlus();

    void zoomMinus();

    void moveLeft();

    void moveRight();

    void moveUp();

    void moveDown();

    void resetView();

protected:
    void initializeGL() override;

    void paintGL() override;

    void resizeGL(int w, int h) override;

private:
    static void on_update(void *ctx);

    void update_cb();

    void create_render_context();

    mpv_handle *m_mpv = nullptr;
    mpv_render_context *m_mpv_gl = nullptr;

    //zoom
    double m_zoom = 0.0;
    double m_panX = 0.0;
    double m_panY = 0.0;
};

#endif // MLT_CALL_API_VIDEOWIDGET_H
