#include <canform.hpp>
#include <gtkmm/gtkmm.hpp>
#include <gtkmm/messagedialog.h>
#include <gtkmm/notebook.h>

namespace CanForm
{

Glib::ustring convert(std::string_view s)
{
    return Glib::ustring(s.data(), s.size());
}

std::string_view convert(const Glib::ustring &s)
{
    return std::string_view(s.data(), s.size());
}

Glib::ustring randomString(size_t n)
{
    Glib::ustring s;
    while (s.size() < n)
    {
        char c;
        do
        {
            c = rand() % 128;
        } while (!std::isalnum(c));
        s.push_back(c);
    }
    return s;
}

Glib::ustring randomString(size_t min, size_t max)
{
    return randomString((rand() % (max - min)) + min);
}

constexpr Gtk::MessageType getType(MessageBoxType type) noexcept
{
    switch (type)
    {
    case MessageBoxType::Warning:
        return Gtk::MESSAGE_WARNING;
    case MessageBoxType::Error:
        return Gtk::MESSAGE_ERROR;
    default:
        return Gtk::MESSAGE_INFO;
    }
}

void showMessageBox(MessageBoxType type, std::string_view title, std::string_view message, void *parent)
{
    Gtk::Window *window = (Gtk::Window *)parent;
    const auto run = [message](Gtk::MessageDialog &dialog) {
        dialog.set_secondary_text(convert(message));
        dialog.run();
    };
    if (window == nullptr)
    {
        Gtk::MessageDialog dialog(convert(title), false, getType(type), Gtk::BUTTONS_OK);
        run(dialog);
    }
    else
    {
        Gtk::MessageDialog dialog(*window, convert(title), false, getType(type), Gtk::BUTTONS_OK);
        run(dialog);
    }
}

void MenuList::show(std::string_view title, void *parent)
{
    Gtk::Window *window = (Gtk::Window *)parent;
    const auto run = [this](Gtk::Dialog &dialog) {
        Gtk::Notebook notebook;
        Gtk::Box *box = dialog.get_content_area();
        box->add(notebook);

        for (auto &menu : menus)
        {
            Gtk::VBox *box = Gtk::manage(new Gtk::VBox());
            for (auto &item : menu.items)
            {
                Gtk::Button *button = Gtk::manage(new Gtk::Button(convert(item->label)));
                button->signal_clicked().connect([&item, &dialog]() {
                    if (item->onClick())
                    {
                        dialog.hide();
                    }
                });
                box->add(*button);
            }
            notebook.append_page(*box, convert(menu.title));
        }
        dialog.add_button("Back", -1);
        dialog.set_default_size(320, 240);
        dialog.show_all_children();
        dialog.run();
    };
    if (window == nullptr)
    {
        Gtk::Dialog dialog(convert(title), true);
        run(dialog);
    }
    else
    {
        Gtk::Dialog dialog(convert(title), *window, true);
        run(dialog);
    }
}

} // namespace CanForm