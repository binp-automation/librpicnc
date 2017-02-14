#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "cnc/driver.h"

typedef struct {
	CNC_Axis *axis;
	GtkWidget *box;
	GtkWidget *left, *right;
	GtkWidget *scale;
	GtkAdjustment *adj;
} AxisView;

AxisView *axis_view_create(CNC_Axis *axis) {
	AxisView *view = malloc(sizeof(AxisView));
	view->axis = axis;

	view->box = gtk_vbox_new(FALSE, 0);

	view->left = gtk_toggle_button_new_with_label("Left");
	// g_signal_connect(view->left, "clicked", G_CALLBACK (cb_left), NULL);
	gtk_box_pack_start(GTK_BOX (view->box), view->left, TRUE, FALSE, 0);
	gtk_widget_show(view->left);

	view->right = gtk_toggle_button_new_with_label("Right");
	// g_signal_connect(view->right, "clicked", G_CALLBACK (cb_right), NULL);
	gtk_box_pack_start(GTK_BOX (view->box), view->right, TRUE, FALSE, 0);
	gtk_widget_show(view->right);

	view->scale = gtk_hscale_new_with_range(0, 1000, 1);
	view->adj = gtk_range_get_adjustment(GTK_RANGE(view->scale));
	gtk_adjustment_set_value(view->adj, 500);
	// g_signal_connect(view->adj, "value_changed", G_CALLBACK (cb_scale), NULL);
	gtk_box_pack_start(GTK_BOX (view->box), view->scale, TRUE, FALSE, 0);
	gtk_widget_show(view->scale);

	gtk_widget_show (view->box);

	return view;
}

void axis_view_destroy(AxisView *view) {
	free(view);
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
	g_print("delete event occurred\n");
	return FALSE;
}

static void destroy(GtkWidget *widget, gpointer data) {
	gtk_main_quit();
}

int	main(int argc, char *argv[]) {
	CNC_Driver *drv = cnc_create_driver("./drivers/test", NULL);
	if (drv == NULL) {
		return 1;
	}

	void *params[] = {"size", (void*) 1000, NULL};
	CNC_Axis *ax = cnc_driver_create_axis(drv, params);
	if (ax == NULL) {
		return 2;
	}

	gtk_init (&argc, &argv);

	AxisView *av = axis_view_create(ax);

	GtkWidget *window;
	int width = 800, height = 600;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), width, height);
	
	g_signal_connect(window, "delete-event", G_CALLBACK (delete_event), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK (destroy), NULL);

	gtk_container_add(GTK_CONTAINER (window), av->box);

	gtk_widget_show(window);
	
	gtk_main();

	axis_view_destroy(av);

	cnc_destroy_axis(ax);

	cnc_destroy_driver(drv);

	return 0;
}