#include <canform.hpp>
#include <filesystem>
#include <fstream>
#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>
#include <memory>

namespace CanForm
{

Glib::ustring convert(const std::string &s)
{
    return Glib::ustring(s);
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
    Gtk::Window *window;

    Gtk::Frame *makeFrame() const
    {
        return Gtk::manage(new Gtk::Frame(convert(name)));
    }

    static bool editTextWithEditor(Glib::RefPtr<Gtk::EntryBuffer>, Gtk::Window *);

  public:
    constexpr FormVisitor(Gtk::Window *w) noexcept : name(), window(w)
    {
    }

    Gtk::Widget *operator()(bool &b)
    {
        Gtk::CheckButton *button = Gtk::manage(new Gtk::CheckButton(convert(name)));
        button->set_active(b);
        button->signal_clicked().connect([&b, button]() { b = button->get_active(); });
        return button;
    }

    Gtk::Widget *operator()(String &s)
    {
        auto frame = makeFrame();
        Gtk::VBox *box = Gtk::manage(new Gtk::VBox());

        Gtk::Entry *entry = Gtk::manage(new Gtk::Entry());
        auto buffer = entry->get_buffer();
        const auto updateText = [&s, buffer]() { s = convert(buffer->get_text()); };

        buffer->set_text(convert(s));
        buffer->signal_inserted_text().connect([updateText](guint, const char *, guint) { updateText(); });
        buffer->signal_deleted_text().connect([updateText](guint, guint) { updateText(); });

        box->pack_start(*entry, Gtk::PACK_EXPAND_PADDING, 10);

        Gtk::Button *button = Gtk::manage(new Gtk::Button("..."));
        button->signal_clicked().connect(
            [buffer, window = window]() { FormVisitor::editTextWithEditor(buffer, window); });
        box->pack_start(*button, Gtk::PACK_EXPAND_WIDGET);

        frame->add(*box);
        return frame;
    }

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true> Gtk::Widget *operator()(T &value)
    {
        auto frame = makeFrame();
        Gtk::SpinButton *button = Gtk::manage(new Gtk::SpinButton());
        button->set_value(value);

        if constexpr (std::is_integral_v<T>)
        {
            button->set_range(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        }
        else
        {
            button->set_range(std::numeric_limits<T>::max() * -1.0, std::numeric_limits<T>::max());
            button->set_digits(6);
        }

        button->set_increments(1, 10);

        button->signal_value_changed().connect([button, &value]() { value = static_cast<T>(button->get_value()); });

        frame->add(*button);
        return frame;
    }

    Gtk::Widget *operator()(Number &n)
    {
        return std::visit(*this, n);
    }

    Gtk::Widget *operator()(StringSelection &selection)
    {
        auto frame = makeFrame();
        Gtk::ComboBoxText *box = Gtk::manage(new Gtk::ComboBoxText());
        for (auto &text : selection.set)
        {
            box->append(convert(text));
        }
        auto s = selection.getSelection();
        if (s)
        {
            box->set_active_text(convert(*s));
        }
        box->signal_changed().connect([box, &selection]() { selection.setSelection(convert(box->get_active_text())); });
        frame->add(*box);
        return frame;
    }

    Gtk::Widget *operator()(StringMap &map)
    {
        auto frame = makeFrame();
        Gtk::VBox *box = Gtk::manage(new Gtk::VBox());
        for (auto &pair : map)
        {
            Gtk::CheckButton *button = Gtk::manage(new Gtk::CheckButton(convert(pair.first)));
            button->set_active(pair.second);
            button->signal_clicked().connect([&pair, button]() { pair.second = button->get_active(); });
            box->add(*button);
        }
        frame->add(*box);
        return frame;
    }

    Gtk::Widget *operator()(MultiForm &multi)
    {
        auto frame = makeFrame();
        Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook());
        notebook->set_scrollable(true);
        frame->add(*notebook);
        size_t selected = 0;
        size_t current = 0;
        for (auto &[n, form] : multi.tabs)
        {
            notebook->append_page(*operator()(form), convert(n));
            if (multi.selected == n)
            {
                selected = current;
            }
            ++current;
        }
        notebook->show_all_children();
        notebook->set_current_page(selected);
        notebook->signal_switch_page().connect([notebook, &multi](Gtk::Widget *widget, guint) {
            if (notebook->is_visible())
            {
                multi.selected = convert(notebook->get_tab_label_text(*widget));
            }
        });
        return frame;
    }

    Gtk::Widget *operator()(Form &form)
    {
        const int rows = std::max(static_cast<size_t>(1), form.datas.size() / 2);
        Gtk::Table *table = Gtk::manage(new Gtk::Table(rows, 2));

        size_t index = 0;
        const Gtk::AttachOptions options = Gtk::FILL | Gtk::EXPAND;
        for (auto &[n, data] : form.datas)
        {
            const int row = index / 2;
            const int column = index % 2;
            name = n;
            table->attach(*std::visit(*this, *data), column, column + 1, row, row + 1, options, options, 10, 10);
            ++index;
        }
        return table;
    }
};

DialogResult executeForm(std::string_view title, Form &form, void *parent)
{
    Gtk::Window *window = (Gtk::Window *)parent;
    const auto run = [&form, window](Gtk::Dialog &dialog) {
        Gtk::Box *box = dialog.get_content_area();
        FormVisitor visitor(&dialog);
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
    FormVisitor visitor(dialog);
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
            result = DialogResult::Cancel;
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

static bool askQuestion(const char *question, Gtk::Window *window)
{
    Glib::ustring string(question);
    const auto run = [window](Gtk::Dialog &dialog) {
        dialog.add_button("No", Gtk::RESPONSE_CANCEL);
        dialog.add_button("Yes", Gtk::RESPONSE_OK);
        dialog.show_all_children();
        return dialog.run() == Gtk::RESPONSE_OK;
    };
    if (window == nullptr)
    {
        Gtk::Dialog dialog(string, true);
        return run(dialog);
    }
    else
    {
        Gtk::Dialog dialog(string, *window, true);
        return run(dialog);
    }
}

static bool openFileAndWait(const TempFile &tempFile, Glib::ustring &s, Gtk::Window *window)
{
    const bool success = tempFile.spawnEditor();
    if (success && askQuestion("Read changes from file?", window))
    {
        return tempFile.read(s);
    }
    return success;
}

bool FormVisitor::editTextWithEditor(Glib::RefPtr<Gtk::EntryBuffer> buffer, Gtk::Window *window)
{
    Glib::ustring string("Edit in Text Editor?");
    const auto run = [buffer, window](Gtk::Dialog &dialog) {
        Glib::ustring currentText(buffer->get_text());

        Gtk::HBox hBox;
        hBox.set_spacing(10);

        Gtk::Label label("Enter temporary file extension:");
        hBox.pack_start(label, Gtk::PACK_SHRINK);

        Gtk::Entry entry;
        entry.get_buffer()->set_text("txt");
        hBox.pack_start(entry, Gtk::PACK_EXPAND_WIDGET);

        Gtk::Box *box = dialog.get_vbox();
        box->add(hBox);

        dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
        dialog.show_all_children();
        if (dialog.run() != Gtk::RESPONSE_OK)
        {
            return false;
        }

        TempFile file(entry.get_buffer()->get_text());

        file.write(buffer->get_text());

        if (!openFileAndWait(file, currentText, window))
        {
            goto error;
        }
        buffer->set_text(currentText);
        return true;

    error:
        showMessageBox(MessageBoxType::Error, "Process Error", "Error creating process or temporary file", window);
        return false;
    };
    if (window == nullptr)
    {
        Gtk::Dialog dialog(string, true);
        return run(dialog);
    }
    else
    {
        Gtk::Dialog dialog(string, *window, true);
        return run(dialog);
    }
}

TempFile::TempFile(const Glib::ustring &ext) : path(randomString(3, 8)), extension(ext)
{
}

TempFile::~TempFile()
{
    std::error_code err;
    std::filesystem::remove(getPath(), err);
}

Glib::ustring TempFile::getName() const
{
    return Glib::ustring::sprintf("%s.%s", path, extension);
}

std::filesystem::path TempFile::getPath() const
{
    return std::filesystem::path(convert(getName()));
}

bool TempFile::spawnEditor() const
{
    std::string openCommand =
#if _WIN32
        "open";
#else
        "xdg-open";
#endif
    std::string args = getName();
    char *argv[3] = {openCommand.data(), args.data(), nullptr};
    return g_spawn_sync(nullptr, argv, nullptr,
                        (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL),
                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

bool TempFile::read(Glib::ustring &s) const
{
    String string(convert(s));
    if (read(string))
    {
        s = convert(string);
        return true;
    }
    return false;
}

bool TempFile::read(String &string) const
{
    std::ifstream file(getPath());
    if (file.is_open())
    {
        string.assign(std::istreambuf_iterator<char>{file}, {});
        return true;
    }
    return false;
}

bool TempFile::write(const Glib::ustring &s) const
{
    const String string(convert(s));
    return write(string);
}

bool TempFile::write(const String &string) const
{
    std::ofstream file(getPath());
    if (file.is_open())
    {
        file << string;
        return true;
    }
    return false;
}
} // namespace CanForm