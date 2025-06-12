#include <canform.hpp>
#include <gtkmm/gtkmm.hpp>
#include <gtkmm/window.hpp>

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

Gtk::Window *getWindow(Gtk::Widget *widget)
{
    if (auto window = dynamic_cast<Gtk::Window *>(widget))
    {
        return window;
    }
    else if (widget == nullptr)
    {
        return nullptr;
    }
    else
    {
        return getWindow(widget->get_parent());
    }
}

std::pair<int, int> getWindowSize(Gtk::Window &window)
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

std::pair<int, int> getMonitorSize(Gtk::Window &window)
{
    auto screen = window.get_screen();
    if (!screen)
    {
        return getWindowSize(window);
    }
    auto w = screen->get_active_window();
    if (!w)
    {
        w = screen->get_root_window();
    }
    if (!w)
    {
        return getWindowSize(window);
    }
    int i = screen->get_monitor_at_window(w);
    Gdk::Rectangle r;
    screen->get_monitor_geometry(i, r);
    return std::make_pair(r.get_width(), r.get_height());
}

constexpr bool nearby(int a, int b) noexcept
{
    return std::abs(a - b) < 10;
}

constexpr int lerp(int from, int to) noexcept
{
    int dist = (to - from) / 2;
    if (nearby(dist, 0))
    {
        return to;
    }
    else
    {
        return from + dist;
    }
}

Gtk::ScrolledWindow *makeScroll(Gtk::Window *window)
{
    Gtk::ScrolledWindow *scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    if (window == nullptr)
    {
        scroll->set_max_content_width(640);
        scroll->set_max_content_height(480);
    }
    else
    {
        auto [w, h] = getMonitorSize(*window);
        scroll->set_max_content_width(w * 3 / 4);
        scroll->set_max_content_height(h * 3 / 4);
    }
    scroll->set_border_width(10);
    scroll->set_propagate_natural_width(true);
    scroll->set_propagate_natural_height(true);
    return scroll;
}

std::pair<int, int> getContentSize(Gtk::Widget &widget)
{
    auto a = widget.get_allocation();
    return std::make_pair(a.get_width(), a.get_height());
}

size_t getChildrenCount(Gtk::Container &container)
{
    size_t count = 0;
    container.foreach ([&count](Gtk::Widget &) { ++count; });
    return count;
}

Gtk::Notebook *makeNotebook()
{
    Gtk::Notebook *notebook = Gtk::make_managed<Gtk::Notebook>();
    notebook->set_scrollable(true);
    notebook->set_show_tabs(true);
    notebook->set_show_border(true);
    return notebook;
}

void showMessageBox(MessageBoxType type, std::string_view title, std::string_view message, void *ptr)
{
    showMessageBox(type, convert(title), convert(message), ptr, []() {});
}

void askQuestion(std::string_view title, std::string_view message, const std::shared_ptr<QuestionResponse> &response,
                 void *ptr)
{
    Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();
    box->set_spacing(10);

    std::pair<Gtk::Widget *, Gtk::Widget *> contents(nullptr, nullptr);
    try
    {
        auto icons = Gtk::IconTheme::get_default();
        contents.first = Gtk::make_managed<Gtk::Image>(icons->load_icon("dialog-question", PixelSize));
    }
    catch (const std::exception &)
    {
    }

    contents.second = Gtk::make_managed<Gtk::Label>(convert(message));

    createWindow(
        convert(title), contents, ptr, Gtk::Stock::NO, [response]() { response->no(); }, Gtk::Stock::YES,
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

    void operator()(MenuState state)
    {
        switch (state)
        {
        case MenuState::Close:
            closeWindow();
            break;
        default:
            break;
        }
    }

    void operator()(MenuItem::NewMenu &&result)
    {
        MenuList::show(result.first, result.second, ptr);
        closeWindow();
    }

    void operator()()
    {
        std::visit(*this, item.onClick(ptr));
    }
};

void MenuList::show(std::string_view title, const std::shared_ptr<MenuList> &menuList, void *ptr)
{
    Gtk::VBox *vbox = Gtk::make_managed<Gtk::VBox>();

    Gtk::SearchBar *bar = Gtk::make_managed<Gtk::SearchBar>();
    vbox->pack_start(*bar, Gtk::PACK_SHRINK);

    Gtk::SearchEntry *entry = Gtk::make_managed<Gtk::SearchEntry>();
    bar->connect_entry(*entry);
    bar->add(*entry);

    Gtk::Notebook *notebook = makeNotebook();
    vbox->pack_start(*notebook, Gtk::PACK_EXPAND_WIDGET);
    for (auto &menu : menuList->menus)
    {
        auto scroll = makeScroll((Gtk::Window *)ptr);

        Gtk::ButtonBox *box = Gtk::make_managed<Gtk::ButtonBox>(Gtk::Orientation::ORIENTATION_VERTICAL);
        box->set_layout(Gtk::ButtonBoxStyle::BUTTONBOX_CENTER);
        box->set_spacing(10);
        for (auto &item : menu.items)
        {
            Gtk::Button *button = Gtk::make_managed<Gtk::Button>(convert(item->label));
            button->signal_clicked().connect(MenuItemHandler(menuList, button, *item, ptr));
            box->add(*button);
        }
        scroll->add(*box);
        notebook->append_page(*scroll, convert(menu.title));
    }

    Gtk::Window *window = createWindow(convert(title), std::make_pair(nullptr, vbox), ptr);
    window->add_events(Gdk::KEY_PRESS_MASK);
    window->signal_key_press_event().connect([bar](GdkEventKey *event) {
        if ((event->state & GDK_CONTROL_MASK) != 0 && event->keyval == GDK_KEY_f)
        {
            bar->set_search_mode(!bar->get_search_mode());
            return true;
        }
        return false;
    });

    auto buffer = entry->get_buffer();
    entry->signal_search_changed().connect([buffer, notebook]() {
        const Glib::ustring text = buffer->get_text();
        for (int i = 0; i < notebook->get_n_pages(); ++i)
        {
            auto page = dynamic_cast<Gtk::ScrolledWindow *>(notebook->get_nth_page(i));
            if (page == nullptr)
            {
                break;
            }
            auto viewport = dynamic_cast<Gtk::Viewport *>(page->get_child());
            if (viewport == nullptr)
            {
                break;
            }
            auto child = dynamic_cast<Gtk::ButtonBox *>(viewport->get_child());
            if (child == nullptr)
            {
                break;
            }
            child->foreach ([&text](Gtk::Widget &widget) {
                Gtk::Button &button = dynamic_cast<Gtk::Button &>(widget);
                const Glib::ustring label = button.get_label();
                const std::string::size_type pos = label.find(text);
                button.set_visible(text.empty() || pos != std::string::npos);
            });
        }
    });
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

} // namespace CanForm