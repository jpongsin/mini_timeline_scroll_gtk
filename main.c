//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info

#include <gtk/gtk.h>
#include "video_fetch.h"
#include "video_implement.h"
#include "timecode_check.h"

////////////////////////////////////////
///prototypes
int main(int argc, char *argv[]);
static void activate(GtkApplication *app, gpointer user_data);
gboolean on_key_pressed(GtkEventController *controller,
                        guint keyval, guint keycode,
                        GdkModifierType state, gpointer user_data);
///////////////////////////////////////////


//takes in the argument, then redirects to GUI
int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    VideoPlayer player = {0};
    /*
    //for use in IDE
    const char *videoFile= 'sample.mp4'
  */
    ////////////////////////////////////////////////
    //check if you entered something.
    if (argc<2) {
        printf("You did not enter anything. Exiting.\n");
        return -1;
    }
    /////////////////////////////////////////////////
    //initialize argv[]
    //transfer from arguments to inside the program
    player.uri=argv[1];

    AVFormatContext *forIOContext = NULL;

    //video stream starts with 0, hence making the starting point -1
    int videoStreamIndex = -1;

    //we obtain the fps from what ffmpeg says
    player.assignedFPS = validateVideo(player.uri, forIOContext, videoStreamIndex);

    //cleanup
    avformat_close_input(&forIOContext);
    g_print("Confirmed FPS for seeking: %.3f\n", player.assignedFPS);

    //start up the GUI
    GtkApplication *app;
    int status;

    //define the GUI, then talk to the GUI and let it run until user closes.
    app = gtk_application_new("com.github.jpongsin.mintimelinescroll", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK (activate), &player);
    status = g_application_run(G_APPLICATION(app), 0, NULL);

    //cleanup
    g_object_unref(app);

    return status;
}

//the gui
static void activate(GtkApplication *app, gpointer user_data) {
    //variables
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hotkey_guides;

    //begin layout
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Mini Timeline Scroll");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 360);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);

    //VERTICAL1
    //hotkey guides
    hotkey_guides = gtk_label_new(""
        "J/K/L - Rewind by 10 Frames/Play-Pause/Forward by 10 Frames. <-/-> - Step Back/Forward by 1 Frame. \n"
        "HOME - Skip Backwards to Start. - F - Enter Fullscreen. M - Mute/Unmute Video.");


    gtk_box_append(GTK_BOX(vbox), hotkey_guides);
    //VERTICAL2
    //Big video.
    VideoPlayer *player = (VideoPlayer *) user_data;
    //initialize a video processor, ffmpeg opening probe
    init_video_processor(player, player->uri);
    //link to sink
    GdkPaintable *paintable;
    g_object_get(player->gtk_sink, "paintable", &paintable, NULL);

    //make a paintable
    GtkWidget *vidFrame = gtk_picture_new_for_paintable(paintable);
    g_object_unref(paintable);

    //layout; make a frame, then assign to the paintable
    GtkWidget *aspect_frame = gtk_aspect_frame_new((float) 0.5, (float) 0.5, (float) (16.0 / 9.0),FALSE);
    gtk_widget_set_hexpand(aspect_frame,TRUE);
    gtk_widget_set_vexpand(aspect_frame,TRUE);
    gtk_aspect_frame_set_child(GTK_ASPECT_FRAME(aspect_frame), vidFrame);
    gtk_box_append(GTK_BOX(vbox), aspect_frame);

    //Show timecode
    //VERTICAL3
    // start off the timecode
    player->timecode_show = gtk_label_new("00:00:00:00");
    gtk_box_append(GTK_BOX(vbox), player->timecode_show);

    // initialize a timecode and update
    gtk_widget_add_tick_callback(player->timecode_show, (GtkTickCallback) on_ui_tick, player, NULL);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
    //end

    //the window has been created
    gtk_widget_realize(window);
    gtk_window_present(GTK_WINDOW(window));

    //autoplays the video you have
    g_idle_add((GSourceFunc) start_playback, player);

    //defines hotkeys: once the GUI is setup, then the hotkeys should be ready to setup
    GtkEventController *key_ctrl = gtk_event_controller_key_new();
    g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), player);
    gtk_widget_add_controller(window, key_ctrl);

}


//hotkeys shortcuts
gboolean on_key_pressed(GtkEventController *controller,
                        guint keyval, guint keycode,
                        GdkModifierType state, gpointer user_data) {

    //define a variable to pass
    VideoPlayer *player = (VideoPlayer *) user_data;

    //true = validate a hotkey
    //false = propagate
    switch (keyval) {
        //j. step back -10 frames
        case GDK_KEY_j:
        case GDK_KEY_J: {
            seek_frames(player, -10);
            return TRUE;
        }
        //k and spacebar. play and pause
        case GDK_KEY_space:
        case GDK_KEY_k: {
            GstState current_state;
            // find the current state of player pipeline....
            gst_element_get_state(player->pipeline, &current_state, NULL, 0);

            if (current_state == GST_STATE_PLAYING) {
                //PAUSE
                // player->current_rate = 1.0;
                gst_element_set_state(player->pipeline, GST_STATE_PAUSED);
                player->is_playing = FALSE;
                g_print("Paused\n");
            } else {
                //PLAY
                // player->current_rate = 1.0;
                gst_element_set_state(player->pipeline, GST_STATE_PLAYING);
                player->is_playing = TRUE;
                g_print("Playing\n");
            }
            return TRUE;
        }
        //left arrow (<-). step backward 1 frame
        case GDK_KEY_Left: {
            //shift+left arrow (<-). step backward 10 frames
            if (state & GDK_SHIFT_MASK) {
                seek_frames(player, -10);
                return TRUE;
            }
            seek_frames(player, -1);
            return TRUE;
        }
            //l: seek +10 frames
        case GDK_KEY_l:
        case GDK_KEY_L: {
            seek_frames(player, 10);
            return TRUE;
        }
            //right arrow (->). step forward 1 frame
        case GDK_KEY_Right: {
            //shift+right arrow (->). step forward 10 frame
            if (state & GDK_SHIFT_MASK) {
                seek_frames(player, 10);
                return TRUE;
            }
            seek_frames(player, 1);
            return TRUE;
        }
        //home key (HOME). rewind to beginning
        case GDK_KEY_Home: {
            seek_begin(player, 0);
            break;
        }
        //f. fullscreen
        case GDK_KEY_f: {
            GtkWidget *tempWindow = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
            if (gtk_window_is_fullscreen(GTK_WINDOW(tempWindow))) {
                gtk_window_unfullscreen(GTK_WINDOW(tempWindow));
            } else {
                gtk_window_fullscreen(GTK_WINDOW(tempWindow));
            }
            return TRUE;
        }
        //m. mute/unmute
        case GDK_KEY_m: {
            //flip, then set
            player->is_muted = !player->is_muted;
            // Set the mute property on the VOLUME element, not the pipeline
            g_object_set(player->volume_stream, "mute", player->is_muted, NULL);
            if (player->is_muted == TRUE) {
                g_print("Muted\n");
            } else {
                g_print("Unmuted\n");
            }

            return TRUE;
        }
            //right bracket "]" = increase speed
        case GDK_KEY_bracketright:
            change_rate(player, 0.25);
            return TRUE;
            //left bracket "[" = decrease speed
        case GDK_KEY_bracketleft:
            change_rate(player, -0.25);
            return TRUE;
            //up "^" = increase volume
        case GDK_KEY_Up: {
            change_volume(player, 0.10);
            return TRUE;
        }
            //down "v" = decrease volume
        case GDK_KEY_Down: {
            change_volume(player, -0.10);
            return TRUE;
        }
        //propagate...
        default:
            return FALSE;
    }
    return TRUE;
}
