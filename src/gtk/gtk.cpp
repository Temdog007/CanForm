#include <canform.hpp>
#include <gtk/gtk.h>

namespace CanForm
{
constexpr GtkMessageType getType(MessageBoxType type) noexcept
{
    switch (type)
    {
    case MessageBoxType::Warning:
        return GTK_MESSAGE_WARNING;
    case MessageBoxType::Error:
        return GTK_MESSAGE_ERROR;
    default:
        return GTK_MESSAGE_INFO;
    }
}
void showMessageBox(MessageBoxType type, std::string_view title, std::string_view message, void *parent)
{
    GtkWindow *window = (GtkWindow *)parent;

    GtkWidget *dialog =
        gtk_message_dialog_new(window, (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                               getType(type), GTK_BUTTONS_OK, "%.*s", (int)title.size(), title.data());
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%.*s", (int)message.size(), message.data());
    gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);
}
} // namespace CanForm