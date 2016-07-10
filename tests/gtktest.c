#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdio.h>

int main(int argc, char **argv) {
	gtk_init (&argc, &argv);
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_NORMAL);
	gtk_widget_set_size_request(window, 400,400);
	gtk_window_set_title(GTK_WINDOW(window), "Opencv tests");
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show(window);
	/* printf("%u\n", GDK_DRAWABLE_XID(gtk_widget_get_window(window))); */
	gtk_main();
	return 0;
}
