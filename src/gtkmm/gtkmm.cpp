#include <canform.hpp>
#include <gtkmm/gtkmm.hpp>
#include <gtkmm/messagedialog.h>

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
    const auto run = [](Gtk::Dialog &dialog) {
        // wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        // wxNotebook *book = new wxNotebook(&dialog, wxID_ANY);
        // sizer->Add(book);
        // for (auto &menu : menus)
        // {
        //     wxPanel *panel = new wxPanel(book);
        //     wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        //     int i = 0;
        //     for (auto &item : menu.items)
        //     {
        //         wxButton *button = new wxButton(panel, i, convert(item->label));
        //         panel->Bind(
        //             wxEVT_BUTTON,
        //             [&dialog, &item](wxCommandEvent &) {
        //                 if (item->onClick())
        //                 {
        //                     dialog.EndModal(wxID_OK);
        //                 }
        //             },
        //             i++);
        //         sizer->Add(button, 1, wxEXPAND);
        //     }
        //     panel->SetSizerAndFit(sizer);
        //     book->AddPage(panel, convert(menu.title));
        // }
        dialog.add_button("Back", -1);
        dialog.set_default_size(320, 240);
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