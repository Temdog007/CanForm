#include <canform.hpp>
#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>
#include <memory>

namespace CanForm
{

Glib::ustring convert(const std::string &s)
{
    return Glib::locale_to_utf8(s);
}

Glib::ustring convert(std::string_view s)
{
    const std::string string(s);
    return convert(string);
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

class FormVisitor
{
  private:
    std::string_view name;

    Gtk::Frame *makeFrame()
    {
        return Gtk::manage(new Gtk::Frame(convert(name)));
    }

  public:
    Gtk::Widget *operator()(bool &b)
    {
        Gtk::CheckButton *button = Gtk::manage(new Gtk::CheckButton(convert(name)));
        button->signal_clicked().connect([&b, button]() { b = button->get_active(); });
        return button;
    }

    Gtk::Widget *operator()(String &)
    {
        return makeFrame();
    }
    Gtk::Widget *operator()(Number &)
    {
        return makeFrame();
    }
    Gtk::Widget *operator()(StringSelection &)
    {
        return makeFrame();
    }
    Gtk::Widget *operator()(StringMap &)
    {
        return makeFrame();
    }
    Gtk::Widget *operator()(MultiForm &multi)
    {
        auto frame = makeFrame();
        Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook());
        notebook->signal_switch_page().connect([notebook, &multi](Gtk::Widget *widget, guint) {
            multi.selected = convert(notebook->get_tab_label_text(*widget));
        });
        frame->add(*notebook);
        for (auto &[n, form] : multi.tabs)
        {
            notebook->append_page(*operator()(form), convert(n));
        }
        return frame;
    }
    Gtk::Widget *operator()(Form &form)
    {
        const int rows = std::max(static_cast<size_t>(1), form.datas.size() / 2);
        Gtk::Table *table = Gtk::manage(new Gtk::Table(rows, 2));

        size_t index = 0;
        for (auto &[n, data] : form.datas)
        {
            const int row = index / 2;
            const int column = index % 2;
            name = n;
            table->attach(*std::visit(*this, *data), column, column + 1, row, row + 1);
            ++index;
        }
        return table;
    }
};

DialogResult executeForm(std::string_view title, Form &form, void *parent)
{
    Gtk::Window *window = (Gtk::Window *)parent;
    const auto run = [&form](Gtk::Dialog &dialog) {
        Gtk::Box *box = dialog.get_content_area();
        FormVisitor visitor;
        box->add(*visitor(form));
        dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
        dialog.set_default_size(320, 240);
        dialog.show_all_children();
        switch (dialog.run())
        {
        case Gtk::RESPONSE_OK:
            return DialogResult::Ok;
        case Gtk::RESPONSE_CANCEL:
            return DialogResult::Cancel;
        default:
            break;
        }
        return DialogResult::Error;
    };
    if (window == nullptr)
    {
        Gtk::Dialog dialog(convert(title), true);
        return run(dialog);
    }
    else
    {
        Gtk::Dialog dialog(convert(title), *window, true);
        return run(dialog);
    }
}

void AsyncForm::show(const std::shared_ptr<AsyncForm> &asyncForm, std::string_view title, void *parent)
{
    if (asyncForm == nullptr)
    {
        return;
    }
    Gtk::Dialog *dialog = nullptr;
    Gtk::Window *window = (Gtk::Window *)parent;

    if (window == nullptr)
    {
        dialog = Gtk::manage(new Gtk::Dialog(convert(title), false));
    }
    else
    {
        dialog = Gtk::manage(new Gtk::Dialog(convert(title), *window, false));
    }

    Gtk::Box *box = dialog->get_content_area();
    FormVisitor visitor;
    box->add(*visitor(asyncForm->form));
    dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog->set_default_size(320, 240);
    dialog->show_all_children();
    dialog->signal_response().connect([dialog, asyncForm](int response) {
        dialog->hide();
        DialogResult result = DialogResult::Error;
        switch (response)
        {
        case Gtk::RESPONSE_OK:
            result = DialogResult::Ok;
            break;
        case Gtk::RESPONSE_CANCEL:
            result = DialogResult::Error;
            break;
        default:
            break;
        }
        return asyncForm->onSubmit(result);
    });
    dialog->run();
}

DialogResult FileDialog::show(FileDialog::Handler &handler, void *parent) const
{
    Gtk::Window *window = (Gtk::Window *)parent;
    const auto run = [this, &handler, parent](Gtk::FileChooserDialog &dialog) {
        dialog.set_current_folder(convert(startDirectory));
        dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        if (directories)
        {
            dialog.add_button("Select", Gtk::RESPONSE_OK);
        }
        else
        {
            dialog.set_current_name(convert(filename));
            dialog.set_select_multiple(multiple);
            dialog.add_button(saving ? Gtk::Stock::SAVE : Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
        }
        switch (dialog.run())
        {
        case Gtk::RESPONSE_OK: {
            for (auto &file : dialog.get_files())
            {
                handler.handle(file->get_path());
            }
            return DialogResult::Ok;
        }
        break;
        case Gtk::RESPONSE_CANCEL:
            return DialogResult::Cancel;
        default:
            break;
        }
        return DialogResult::Error;
    };
    Gtk::FileChooserAction action;
    if (directories)
    {
        action = Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER;
    }
    else if (saving)
    {
        action = Gtk::FILE_CHOOSER_ACTION_SAVE;
    }
    else
    {
        action = Gtk::FILE_CHOOSER_ACTION_OPEN;
    }
    if (window == nullptr)
    {
        Gtk::FileChooserDialog dialog(convert(title), action);
        return run(dialog);
    }
    else
    {
        Gtk::FileChooserDialog dialog(*window, convert(title), action);
        return run(dialog);
    }
}

} // namespace CanForm