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

std::string convert(const Glib::ustring &s)
{
    return std::string(s);
}

std::string_view toView(const Glib::ustring &s)
{
    return std::string(s.data(), s.size());
}

static std::pair<int, int> getWindowSize(Gtk::Window &window)
{
    auto w = window.get_transient_for();
    if (w == nullptr)
    {
        std::pair<int, int> pair(0, 0);
        window.get_size(pair.first, pair.second);
        return pair;
    }
    else
    {
        return getWindowSize(*w);
    }
}

static std::pair<int, int> getMonitorSize(Gtk::Window &window)
{
    auto screen = window.get_screen();
    if (!screen)
    {
        return getWindowSize(window);
    }
    auto w = screen->get_active_window();
    if (!w)
    {
        return getWindowSize(window);
    }
    int i = screen->get_monitor_at_window(w);
    Gdk::Rectangle r;
    screen->get_monitor_geometry(i, r);
    return std::make_pair(r.get_width(), r.get_height());
}

static void setDialogSize(Gtk::Window &window)
{
    auto [w, h] = getMonitorSize(window);
    window.set_default_size(w / 2, h / 2);
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

bool askQuestion(std::string_view title, std::string_view message, void *parent)
{
    Gtk::Window *window = (Gtk::Window *)parent;
    const auto run = [message](Gtk::MessageDialog &dialog) {
        dialog.set_secondary_text(convert(message));
        return dialog.run() == Gtk::RESPONSE_YES;
    };
    if (window == nullptr)
    {
        Gtk::MessageDialog dialog(convert(title), true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
        return run(dialog);
    }
    else
    {
        Gtk::MessageDialog dialog(*window, convert(title), true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
        return run(dialog);
    }
}

void showPopupUntil(std::string_view message, const std::shared_ptr<Awaiter> &awaiter, size_t checkRate, void *ptr)
{
    Gtk::Window *parent = (Gtk::Window *)ptr;
    Gtk::Window *window = Gtk::manage(new Gtk::Window(Gtk::WINDOW_POPUP));
    if (parent != nullptr)
    {
        window->set_transient_for(*parent);
    }
    window->set_urgency_hint(true);
    window->set_keep_above(true);
    window->set_modal(true);
    window->set_default_size(320, 240);
    window->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
    Gtk::Label *label = Gtk::manage(new Gtk::Label(convert(message)));
    window->add(*label);
    window->show_all_children();
    window->show();

    Glib::signal_timeout().connect(
        [window, awaiter]() {
            if (awaiter->isDone())
            {
                window->hide();
                return false;
            }
            return true;
        },
        checkRate);
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
    size_t columns;
    Gtk::Window *window;

    Gtk::Frame *makeFrame() const
    {
        return Gtk::manage(new Gtk::Frame(convert(name)));
    }

    template <typename B> void addSyncFile(Gtk::Box &box, B buffer) const
    {
        Gtk::HBox *hBox = Gtk::manage(new Gtk::HBox());

        SyncButton *button = Gtk::manage(new SyncButton(buffer));
        hBox->add(*button);

        box.pack_start(*hBox, Gtk::PACK_SHRINK);
    }

  public:
    constexpr FormVisitor(size_t c, Gtk::Window *w) noexcept : name(), columns(c), window(w)
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

        addSyncFile(*box, buffer);

        frame->add(*box);
        return frame;
    }

    Gtk::Widget *operator()(ComplexString &s)
    {
        auto frame = makeFrame();
        Gtk::VBox *box = Gtk::manage(new Gtk::VBox());

        Gtk::TextView *entry = Gtk::manage(new Gtk::TextView());
        entry->set_pixels_above_lines(5);
        entry->set_pixels_below_lines(5);
        entry->set_bottom_margin(10);
        auto buffer = entry->get_buffer();

        buffer->set_text(convert(s.string));
        buffer->signal_changed().connect([&s, buffer]() { s.string = convert(buffer->get_text()); });

        box->pack_start(*entry, Gtk::PACK_EXPAND_WIDGET, 10);

        addSyncFile(*box, buffer);

        Gtk::Expander *expander = Gtk::manage(new Gtk::Expander());
        expander->set_label("Text Insertions");
        expander->set_label_fill(true);
        expander->set_resize_toplevel(true);

        Gtk::VBox *box2 = Gtk::manage(new Gtk::VBox());
        for (auto &[label, set] : s.map)
        {
            Gtk::Frame *frame = Gtk::manage(new Gtk::Frame(convert(label)));
            Gtk::FlowBox *flow = Gtk::manage(new Gtk::FlowBox());
            for (auto &s : set)
            {
                Gtk::Button *button = Gtk::manage(new Gtk::Button(convert(s)));
                button->signal_clicked().connect([buffer, entry, &s]() {
                    buffer->insert_at_cursor(s.data(), s.data() + s.size());
                    entry->grab_focus();
                });
                flow->add(*button);
            }
            frame->add(*flow);
            box2->add(*frame);
        }

        expander->add(*box2);
        box->pack_start(*expander, Gtk::PACK_EXPAND_PADDING);

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
        Gtk::FlowBox *box = Gtk::manage(new Gtk::FlowBox());
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
        const int rows = std::max(static_cast<size_t>(1), form.datas.size() / columns);
        Gtk::Table *table = Gtk::manage(new Gtk::Table(rows, columns));

        size_t index = 0;
        const Gtk::AttachOptions options = Gtk::SHRINK;
        for (auto &[n, data] : form.datas)
        {
            const int row = index / columns;
            const int column = index % columns;
            name = n;
            table->attach(*std::visit(*this, *data), column, column + 1, row, row + 1, options, options, 10, 10);
            ++index;
        }
        return table;
    }
};

static Gtk::ScrolledWindow *makeScroll()
{
    Gtk::ScrolledWindow *scroll = Gtk::manage(new Gtk::ScrolledWindow());
    scroll->set_min_content_width(320);
    scroll->set_min_content_height(240);
    scroll->set_propagate_natural_width(true);
    scroll->set_propagate_natural_height(true);
    return scroll;
}

DialogResult executeForm(std::string_view title, Form &form, size_t columns, void *parent)
{
    Gtk::Window *window = (Gtk::Window *)parent;
    const auto run = [&form, window, columns](Gtk::Dialog &dialog) {
        Gtk::Box *box = dialog.get_content_area();
        FormVisitor visitor(columns, &dialog);
        auto scroll = makeScroll();
        scroll->add(*visitor(form));
        box->pack_start(*scroll, true, true);
        dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
        setDialogSize(dialog);
        dialog.set_resizable(true);
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

void AsyncForm::show(const std::shared_ptr<AsyncForm> &asyncForm, std::string_view title, size_t columns, void *parent)
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
    FormVisitor visitor(columns, dialog);
    auto scroll = makeScroll();
    scroll->add(*visitor(asyncForm->form));
    box->pack_start(*scroll, true, true);
    dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    setDialogSize(*dialog);
    dialog->set_resizable(true);
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

TempFile::TempFile(const Glib::ustring &ext) : path(), extension(ext), timePoint()
{
    auto random = randomString(3, 8);
    path.assign(random.c_str(), random.size());
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
    std::error_code err;
    std::filesystem::path path = std::filesystem::temp_directory_path(err);
    path /= convert(getName());
    return path;
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

bool TempFile::write(const Glib::ustring &s)
{
    const String string(convert(s));
    return write(string);
}

bool TempFile::write(const String &string)
{
    const auto path = getPath();
    {
        std::ofstream file(path);
        if (file.is_open())
        {
            file << string;
            goto storeWrite;
        }
    }
    return false;
storeWrite:
    std::error_code err;
    timePoint = std::filesystem::last_write_time(path, err);
    if (err)
    {
        return false;
    }
    return true;
}

void TempFile::open() const
{
    std::vector<std::string> argv;
#if _WIN32
    argv.emplace_back("open");
#else
    argv.emplace_back("xdg-open");
#endif
    auto path = getPath().string();
    path.insert(0, "file://");
    argv.emplace_back(std::move(path));
    Glib::spawn_async("", argv,
                      Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_STDOUT_TO_DEV_NULL | Glib::SPAWN_STDERR_TO_DEV_NULL);
}

bool TempFile::changed() const
{
    std::error_code err;
    return timePoint != std::filesystem::last_write_time(getPath(), err);
}

} // namespace CanForm