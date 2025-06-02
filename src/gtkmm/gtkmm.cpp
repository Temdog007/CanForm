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
        return getWindowSize(window);
    }
    int i = screen->get_monitor_at_window(w);
    Gdk::Rectangle r;
    screen->get_monitor_geometry(i, r);
    return std::make_pair(r.get_width(), r.get_height());
}

Gtk::ScrolledWindow *makeScroll(Gtk::Window *window)
{
    Gtk::ScrolledWindow *scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    const auto setFromMonitor = [scroll, window]() {
        if (window == nullptr)
        {
            scroll->set_min_content_width(320);
            scroll->set_min_content_height(240);
            scroll->set_max_content_width(640);
            scroll->set_max_content_height(480);
        }
        else
        {
            auto [w, h] = getMonitorSize(*window);
            scroll->set_min_content_width(std::max(w / 2, 320));
            scroll->set_min_content_height(std::max(h / 2, 240));
            scroll->set_max_content_width(w * 5 / 6);
            scroll->set_max_content_height(h * 5 / 6);
        }
    };
    if (window == nullptr)
    {
        setFromMonitor();
    }
    else
    {
        auto [w, h] = getWindowSize(*window);
        if (w < 320 || h < 240)
        {
            setFromMonitor();
        }
        else
        {
            scroll->set_min_content_width(w / 2);
            scroll->set_min_content_height(h / 2);
            scroll->set_max_content_width(w * 5 / 6);
            scroll->set_max_content_height(h * 5 / 6);
        }
    }
    scroll->set_propagate_natural_width(true);
    scroll->set_propagate_natural_height(true);
    Glib::signal_idle().connect([scroll]() {
        int width = 4000;
        int height = 4000;
        scroll->foreach ([&width, &height](Gtk::Widget &widget) {
            Gtk::Requisition minimum;
            Gtk::Requisition natural;
            widget.get_preferred_size(minimum, natural);
            width = std::min({width, minimum.width, natural.width});
            height = std::min({height, minimum.height, natural.height});
        });
        width = std::max(width, 100);
        height = std::max(height, 100);
        scroll->set_min_content_width(width);
        scroll->set_min_content_height(height);
        Gtk::Window *window = getWindow(scroll);
        if (window)
        {
            auto [w, h] = getMonitorSize(*window);
            width = std::min(w * 5 / 6, width) + 100;
            height = std::min(h * 5 / 6, height) + 100;

            window->resize(width, height);
            auto parent = window->get_transient_for();
            if (parent)
            {
                int x, y, w, h;
                parent->get_position(x, y);
                parent->get_size(w, h);

                width = std::min(width, w);
                height = std::min(height, h);
                window->move(x + (w - width) / 2, y + (h - height) / 2);
            }
        }
        return false;
    });
    return scroll;
}

Gtk::Notebook *makeNotebook()
{
    Gtk::Notebook *notebook = Gtk::make_managed<Gtk::Notebook>();
    notebook->set_scrollable(true);
    notebook->set_show_tabs(true);
    notebook->set_show_border(true);
    return notebook;
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

    void operator()(bool b)
    {
        if (b)
        {
            closeWindow();
        }
    }

    void operator()(MenuItem::NewMenu &&result)
    {
        MenuList::show(result.first, result.second, ptr);
        closeWindow();
    }

    void operator()()
    {
        std::visit(*this, item.onClick());
    }
};

void MenuList::show(std::string_view title, const std::shared_ptr<MenuList> &menuList, void *ptr)
{
    Gtk::Notebook *notebook = makeNotebook();
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

    createWindow(convert(title), std::make_pair(nullptr, notebook), ptr);
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