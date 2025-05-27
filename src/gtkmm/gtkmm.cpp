#include <canform.hpp>
#include <filesystem>
#include <fstream>
#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>
#include <memory>

#if __WIN32
#include <Windows.h>
#endif

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

static Gtk::ScrolledWindow *makeScroll(Gtk::Window *window)
{
    Gtk::ScrolledWindow *scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    if (window == nullptr)
    {
        scroll->set_min_content_width(320);
        scroll->set_min_content_height(240);
    }
    else
    {
        {
            auto [w, h] = getMonitorSize(*window);
            scroll->set_min_content_width(w / 2);
            scroll->set_min_content_height(h / 2);
        }
        {
            auto [w, h] = getWindowSize(*window);
            if (w < 0)
            {
                scroll->set_max_content_width(w * 5 / 6);
            }
            if (h < 0)
            {
                scroll->set_max_content_height(h * 5 / 6);
            }
        }
    }
    scroll->set_propagate_natural_width(true);
    scroll->set_propagate_natural_height(true);
    return scroll;
}

constexpr int PixelSize = 32;

constexpr const char *getIconName(MessageBoxType type) noexcept
{
    switch (type)
    {
    case MessageBoxType::Warning:
        return "messagebox_warning";
    case MessageBoxType::Error:
        return "messagebox_critical";
    default:
        break;
    }
    return "messagebox_info";
}

static void makeButtons(Gtk::Window *, Gtk::HBox *)
{
}

template <typename T, typename F, typename... Args>
static void makeButtons(Gtk::Window *window, Gtk::HBox *box, T icon, F func, Args &&...args)
{
    Gtk::Button *button = Gtk::make_managed<Gtk::Button>(icon);
    button->signal_clicked().connect([window, func = std::move(func)]() {
        window->hide();
        func();
        delete window;
    });
    box->pack_start(*button, Gtk::PACK_EXPAND_PADDING);
    makeButtons(window, box, std::forward<Args>(args)...);
}

std::pair<int, int> getSize(Gtk::Container &container)
{
    std::pair<int, int> pair(0, 0);
    for (Gtk::Widget *child : container.get_children())
    {
        pair.first += child->get_allocated_width();
        pair.second += child->get_allocated_height();
    }
    return pair;
}

template <typename... Args>
static Gtk::Window *createWindow(Gtk::WindowType type, std::string_view title, Gtk::Widget *content, void *ptr,
                                 Args &&...args)
{
    Gtk::Window *parent = (Gtk::Window *)ptr;
    Gtk::Window *window = new Gtk::Window(type);
    window->set_modal(true);
    window->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    window->set_title(convert(title));

    Gtk::VBox *vBox = Gtk::make_managed<Gtk::VBox>();
    window->add(*vBox);

    Gtk::ScrolledWindow *scroll = makeScroll(window);
    scroll->add(*content);
    vBox->pack_start(*scroll, Gtk::PACK_EXPAND_PADDING);

    Gtk::Separator *separator = Gtk::make_managed<Gtk::Separator>();
    vBox->pack_start(*separator, Gtk::PACK_SHRINK);

    Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();
    makeButtons(window, hBox, std::forward<Args>(args)...);
    vBox->pack_start(*hBox, Gtk::PACK_EXPAND_PADDING);

    std::pair<int, int> pair;
    if (parent == nullptr)
    {
        pair = getSize(*vBox);
    }
    else
    {
        window->set_transient_for(*parent);
        window->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
        pair = getWindowSize(*parent);
    }
    window->set_default_size(pair.first * 5 / 6, pair.second * 5 / 6);

    window->show_all_children();
    window->show();
    return window;
}

template <typename... Args>
static Gtk::Window *createWindow(std::string_view title, Gtk::Widget *content, void *ptr, Args &&...args)
{
    return createWindow(Gtk::WINDOW_TOPLEVEL, title, content, ptr, std::forward<Args>(args)...);
}

void showMessageBox(MessageBoxType type, std::string_view title, std::string_view message, void *ptr)
{
    Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();
    box->set_spacing(10);

    try
    {
        auto icons = Gtk::IconTheme::get_default();
        Gtk::Image *image = Gtk::make_managed<Gtk::Image>(icons->load_icon(getIconName(type), PixelSize));
        box->pack_start(*image, Gtk::PACK_SHRINK);
    }
    catch (const std::exception &)
    {
    }

    Gtk::Label *label = Gtk::make_managed<Gtk::Label>(convert(message));
    box->pack_start(*label, Gtk::PACK_SHRINK);

    createWindow(title, box, ptr, Gtk::Stock::OK, []() {});
}

void askQuestion(std::string_view title, std::string_view message, const std::shared_ptr<QuestionResponse> &response,
                 void *ptr)
{
    Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();
    box->set_spacing(10);

    try
    {
        auto icons = Gtk::IconTheme::get_default();
        Gtk::Image *image = Gtk::make_managed<Gtk::Image>(icons->load_icon("dialog-question", PixelSize));
        box->pack_start(*image, Gtk::PACK_SHRINK);
    }
    catch (const std::exception &)
    {
    }

    Gtk::Label *label = Gtk::make_managed<Gtk::Label>(convert(message));
    box->pack_start(*label, Gtk::PACK_SHRINK);

    createWindow(
        title, box, ptr, Gtk::Stock::NO, [response]() { response->no(); }, Gtk::Stock::YES,
        [response]() { response->yes(); });
}

void showPopupUntil(std::string_view message, const std::shared_ptr<Awaiter> &awaiter, size_t checkRate, void *ptr)
{
    Gtk::Window *parent = (Gtk::Window *)ptr;
    Gtk::Window *window = new Gtk::Window(Gtk::WINDOW_POPUP);

    std::pair<int, int> pair;
    if (parent == nullptr)
    {
        pair.first = 373;
        pair.second = 280;
    }
    else
    {
        window->set_transient_for(*parent);
        window->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
        pair = getWindowSize(*parent);
    }
    window->set_default_size(pair.first * 5 / 6, pair.second * 5 / 6);

    window->set_urgency_hint(true);
    window->set_keep_above(true);
    window->set_modal(true);
    Gtk::Label *label = Gtk::make_managed<Gtk::Label>(convert(message));
    window->add(*label);
    window->show_all_children();
    window->show();

    Glib::signal_timeout().connect(
        [window, awaiter]() {
            if (awaiter->isDone())
            {
                window->hide();
                delete window;
                return false;
            }
            return true;
        },
        checkRate);
}

struct MenuItemHandler
{
    std::shared_ptr<MenuList> menuList;
    Gtk::Button *button;
    MenuItem &item;
    void *ptr;

    MenuItemHandler(const std::shared_ptr<MenuList> &m, Gtk::Button *b, MenuItem &i, void *p)
        : menuList(m), button(b), item(i), ptr(p)
    {
    }

    void closeWindow()
    {
        Gtk::Widget *w = button->get_parent();
        while (true)
        {
            Gtk::Widget *w2 = w->get_parent();
            if (w2 == nullptr)
            {
                break;
            }
            w = w2;
        }
        w->hide();
        delete w;
    }

    void operator()(bool b)
    {
        if (b)
        {
            closeWindow();
        }
    }

    void operator()(MenuItem::NewMenu &&result)
    {
        showMenu(result.first, result.second, ptr);
        closeWindow();
    }

    void operator()()
    {
        std::visit(*this, item.onClick());
    }
};

void showMenu(std::string_view title, const std::shared_ptr<MenuList> &menuList, void *ptr)
{
    Gtk::Notebook *notebook = Gtk::make_managed<Gtk::Notebook>();
    for (auto &menu : menuList->menus)
    {
        Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();
        for (auto &item : menu.items)
        {
            Gtk::Button *button = Gtk::make_managed<Gtk::Button>(convert(item->label));
            button->signal_clicked().connect(MenuItemHandler(menuList, button, *item, ptr));
            box->add(*button);
        }
        notebook->append_page(*box, convert(menu.title));
    }

    createWindow(title, notebook, ptr);
}

class FormVisitor
{
  private:
    std::string_view name;
    size_t columns;

    Gtk::Frame *makeFrame() const
    {
        return Gtk::make_managed<Gtk::Frame>(convert(name));
    }

    template <typename B> void addSyncFile(Gtk::Box &box, B buffer) const
    {
        Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();

        SyncButton *button = Gtk::make_managed<SyncButton>(convert(name), buffer);
        hBox->add(*button);

        box.pack_start(*hBox, Gtk::PACK_SHRINK);
    }

  public:
    constexpr FormVisitor(size_t c) noexcept : name(), columns(c)
    {
    }

    Gtk::Widget *operator()(bool &b)
    {
        Gtk::CheckButton *button = Gtk::make_managed<Gtk::CheckButton>(convert(name));
        button->set_active(b);
        button->signal_clicked().connect([&b, button]() { b = button->get_active(); });
        return button;
    }

    Gtk::Widget *operator()(String &s)
    {
        auto frame = makeFrame();
        Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();

        Gtk::Entry *entry = Gtk::make_managed<Gtk::Entry>();
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
        Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();

        Gtk::TextView *entry = Gtk::make_managed<Gtk::TextView>();
        entry->set_pixels_above_lines(5);
        entry->set_pixels_below_lines(5);
        entry->set_bottom_margin(10);
        auto buffer = entry->get_buffer();

        buffer->set_text(convert(s.string));
        buffer->signal_changed().connect([&s, buffer]() { s.string = convert(buffer->get_text()); });

        box->pack_start(*entry, Gtk::PACK_EXPAND_WIDGET, 10);

        addSyncFile(*box, buffer);

        Gtk::Expander *expander = Gtk::make_managed<Gtk::Expander>();
        expander->set_label("Text Insertions");
        expander->set_label_fill(true);
        expander->set_resize_toplevel(true);

        Gtk::VBox *box2 = Gtk::make_managed<Gtk::VBox>();
        for (auto &[label, set] : s.map)
        {
            Gtk::Frame *frame = Gtk::make_managed<Gtk::Frame>(convert(label));
            Gtk::FlowBox *flow = Gtk::make_managed<Gtk::FlowBox>();
            for (auto &s : set)
            {
                Gtk::Button *button = Gtk::make_managed<Gtk::Button>(convert(s));
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
        Gtk::SpinButton *button = Gtk::make_managed<Gtk::SpinButton>();
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

    enum class RepositionMode
    {
        Swap,
        MoveBefore,
        MoveAfter,
    };

    template <typename T> static void reposition(T &list, size_t oldPosition, size_t newPosition)
    {
        auto iter = list.begin() + oldPosition;
        auto item = std::move(*iter);
        list.erase(iter);
        if (newPosition < list.size())
        {
            list.emplace(list.begin() + newPosition, std::move(item));
        }
        else
        {
            list.emplace_back(std::move(item));
        }
    }

    static void setStringList(Gtk::VBox *box, std::shared_ptr<RepositionMode> mode, StringList &list)
    {
        box->foreach ([box](Gtk::Widget &widget) { box->remove(widget); });
        using Pair = std::optional<size_t>;
        std::shared_ptr<Pair> ptr = std::make_shared<Pair>();
        for (size_t i = 0; i < list.size(); ++i)
        {
            Gtk::ToggleButton *button = Gtk::make_managed<Gtk::ToggleButton>(convert(list[i].first));
            button->signal_toggled().connect([&list, button, box, mode, ptr, i]() {
                Pair &pair = *ptr;
                if (!button->get_active())
                {
                    if (pair == i)
                    {
                        pair.reset();
                        return;
                    }
                }
                if (!pair)
                {
                    pair.emplace(i);
                    return;
                }
                if (*pair == i)
                {
                    return;
                }
                switch (*mode)
                {
                case RepositionMode::MoveBefore:
                    reposition(list, *pair, i);
                    break;
                case RepositionMode::MoveAfter:
                    reposition(list, *pair, i + 1);
                    break;
                default:
                    std::swap(list[*pair], list[i]);
                    break;
                }
                setStringList(box, mode, list);
            });
            box->add(*button);
        }
        box->show_all_children();
    }

    Gtk::Widget *operator()(StringList &list)
    {
        auto frame = makeFrame();
        Gtk::VBox *vBox = Gtk::make_managed<Gtk::VBox>();

        Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();
        vBox->add(*hBox);

        Gtk::RadioButtonGroup group;

        std::shared_ptr<RepositionMode> mode = std::make_shared<RepositionMode>();

        Gtk::RadioButton *button = Gtk::make_managed<Gtk::RadioButton>(group, "Swap");
        button->signal_clicked().connect([mode]() { *mode = RepositionMode::Swap; });
        hBox->add(*button);

        button = Gtk::make_managed<Gtk::RadioButton>(group, "Move Before");
        button->signal_clicked().connect([mode]() { *mode = RepositionMode::MoveBefore; });
        hBox->add(*button);

        button = Gtk::make_managed<Gtk::RadioButton>(group, "Move After");
        button->signal_clicked().connect([mode]() { *mode = RepositionMode::MoveAfter; });
        hBox->add(*button);

        Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();
        setStringList(box, mode, list);
        vBox->add(*box);

        frame->add(*vBox);
        return frame;
    }

    Gtk::Widget *operator()(StringSelection &selection)
    {
        auto frame = makeFrame();
        Gtk::ComboBoxText *box = Gtk::make_managed<Gtk::ComboBoxText>();
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
        Gtk::FlowBox *box = Gtk::make_managed<Gtk::FlowBox>();
        for (auto &pair : map)
        {
            Gtk::CheckButton *button = Gtk::make_managed<Gtk::CheckButton>(convert(pair.first));
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
        Gtk::Notebook *notebook = Gtk::make_managed<Gtk::Notebook>();
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
        Gtk::Table *table = Gtk::make_managed<Gtk::Table>(rows, columns);

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

void FormExecute::execute(std::string_view title, const std::shared_ptr<FormExecute> &formExecute, size_t columns,
                          void *ptr)
{
    FormVisitor visitor(columns);
    createWindow(
        title, visitor(formExecute->form), ptr, Gtk::Stock::OK, [formExecute]() { formExecute->ok(); },
        Gtk::Stock::CANCEL, [formExecute]() { formExecute->cancel(); });
}

void FileDialog::show(const std::shared_ptr<FileDialog::Handler> &handler, void *ptr) const
{
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
    Gtk::Window *parent = (Gtk::Window *)ptr;
    Gtk::FileChooserDialog *dialog = nullptr;
    if (parent == nullptr)
    {
        dialog = new Gtk::FileChooserDialog(convert(title), action);
    }
    else
    {
        dialog = new Gtk::FileChooserDialog(*parent, convert(title), action);
    }
    dialog->set_do_overwrite_confirmation(true);
    dialog->set_urgency_hint(true);
    dialog->set_modal(true);
    dialog->set_current_folder(convert(startDirectory));
    dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    if (saving)
    {
        dialog->set_current_name(convert(filename));
    }
    if (directories)
    {
        dialog->add_button("Select", Gtk::RESPONSE_OK);
    }
    else
    {
        dialog->set_select_multiple(multiple);
        dialog->add_button(saving ? Gtk::Stock::SAVE : Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
    }
    dialog->signal_response().connect([dialog, handler](int response) {
        switch (response)
        {
        case Gtk::RESPONSE_OK:
            for (auto &file : dialog->get_files())
            {
                handler->handle(file->get_path());
            }
            break;
        case Gtk::RESPONSE_CANCEL:
            handler->canceled();
            break;
        default:
            break;
        }
        dialog->hide();
        delete dialog;
    });
    dialog->present();
}

TempFile::TempFile(const Glib::ustring &ext) : filename(), extension(ext), timePoint()
{
    randomString(filename, 3, 8);
}

Glib::ustring TempFile::getName() const
{
    return Glib::ustring::sprintf("%s.%s", filename, extension);
}

std::filesystem::path TempFile::getPath() const
{
    std::error_code err;
    std::filesystem::path path = std::filesystem::temp_directory_path(err);
    path /= convert(getName());
    return path;
}

bool TempFile::read(Glib::ustring &s, bool updateTimePoint) const
{
    std::string string(convert(s));
    if (read(string, updateTimePoint))
    {
        s = convert(string);
        return true;
    }
    return false;
}

bool TempFile::read(std::string &string, bool updateTimePoint) const
{
    const auto path = getPath();
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            std::error_code err;
            const auto fileSize = std::filesystem::file_size(path, err);
            string.resize(fileSize, '\0');
            file.read(string.data(), fileSize);
            goto storeWrite;
        }
    }
    return false;
storeWrite:
    if (updateTimePoint)
    {
        std::error_code err;
        timePoint = std::filesystem::last_write_time(path, err);
        if (err)
        {
            return false;
        }
    }
    return true;
}

bool TempFile::write(const Glib::ustring &s) const
{
    const std::string string(s);
    return write(string);
}

bool TempFile::write(const std::string &string) const
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

bool TempFile::changed() const
{
    std::error_code err;
    return timePoint != std::filesystem::last_write_time(getPath(), err);
}

bool TempFile::openFile(std::string_view filePath)
{
    try
    {
        std::error_code err;
        auto path = std::filesystem::temp_directory_path(err);
        if (!filePath.empty())
        {
            path /= filePath;
        }
        auto pathString = path.string();
        pathString.insert(0, "file://");
#if __WIN32
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (hr == RPC_E_CHANGED_MODE)
        {
            hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        }
        if (hr != S_FALSE && FAILED(hr))
        {
            return false;
        }
        HINSTANCE rc = ShellExecute(nullptr, "open", pathString.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        CoUninitialize();
        return rc <= (HINSTANCE)32;
#else
        std::vector<std::string> argv;
        argv.emplace_back("xdg-open");
        argv.emplace_back(std::move(pathString));
        Glib::spawn_async("", argv,
                          Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_STDOUT_TO_DEV_NULL | Glib::SPAWN_STDERR_TO_DEV_NULL);
#endif
        return true;
    }
    catch (const std::exception &e)
    {
        showMessageBox(MessageBoxType::Error, "Failed to open text editor", e.what());
        return false;
    }
}

bool TempFile::open() const
{
    return openFile(convert(getName()));
}

bool TempFile::openTempDirectory()
{
    return openFile("");
}

} // namespace CanForm