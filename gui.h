#pragma once

#include <gtk/gtk.h>

#include "gpio.h"

static void cb_run(GtkWidget *widget, gpointer data) {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
        gpio_start();
    } else {
        gpio_stop();
    }
}

static void cb_dir(GtkWidget *widget, gpointer data) {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
        gpio_set_dir(1);
    } else {
        gpio_set_dir(0);
    }
}

static void cb_clock(GtkWidget *widget, gpointer data) {
    int clock = gtk_adjustment_get_value(GTK_ADJUSTMENT(widget));
    gpio_set_clock(clock);
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    g_print("delete event occurred\n");
    return FALSE;
}

static void destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

int gui_main(int argc, char *argv[]) {
    GtkWidget *window;
    int width = 400, height = 100;
    
    GtkWidget *box;

    GtkWidget *button_run;
    GtkWidget *button_dir;

    GtkWidget *hscale_clock;

    
    gtk_init (&argc, &argv);
    
    /* Window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    
    g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (window, "destroy", G_CALLBACK (destroy), NULL);
    
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    /* Box */
    box = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), box);
    
    /* Button */
    button_run = gtk_toggle_button_new_with_label("Run");
    g_signal_connect (button_run, "clicked", G_CALLBACK (cb_run), NULL);
    gtk_box_pack_start(GTK_BOX (box), button_run, TRUE, FALSE, 0);
    gtk_widget_show (button_run);

    button_dir = gtk_toggle_button_new_with_label("Dir");
    g_signal_connect (button_dir, "clicked", G_CALLBACK (cb_dir), NULL);
    gtk_box_pack_start(GTK_BOX (box), button_dir, TRUE, FALSE, 0);
    gtk_widget_show (button_dir);

    /* Scale */
    hscale_clock = gtk_hscale_new_with_range(2, 2000, 1);
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(hscale_clock));
    gtk_adjustment_set_value(adj, 500);
    g_signal_connect (adj, "value_changed", G_CALLBACK (cb_clock), NULL);
    gtk_box_pack_start(GTK_BOX (box), hscale_clock, TRUE, FALSE, 0);
    gtk_widget_show(hscale_clock);

    gtk_widget_show (box);

    gtk_widget_show (window);
    
    gtk_main ();
    
    return 0;
}