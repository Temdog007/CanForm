#include <canform.hpp>
#include <gtk/gtk.h>

using namespace CanForm;

void show_message(GtkWindow *, gpointer);
void activate(GtkApplication *, gpointer);

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("oss.canform.test", (GApplicationFlags)0);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    const int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

void show_message(GtkWindow *window, gpointer)
{
    showMessageBox(MessageBoxType::Information, "Message", "Clicked the button", window);
}

void activate(GtkApplication *app, gpointer)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "CanForm GTK Test");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(window), button_box);

    GtkWidget *button = gtk_button_new_with_label("Hello World");
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(show_message), window);
    gtk_container_add(GTK_CONTAINER(button_box), button);

    gtk_widget_show_all(window);
}
