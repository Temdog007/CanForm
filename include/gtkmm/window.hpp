#pragma once

#include <canform.hpp>
#include <gtkmm.h>

namespace CanForm
{
constexpr const char *getIconName(MessageBoxType type) noexcept
{
    switch (type)
    {
    case MessageBoxType::Warning:
        return "dialog-warning";
    case MessageBoxType::Error:
        return "dialog-error";
    default:
        break;
    }
    return "dialog-information";
}

extern Gtk::ScrolledWindow *makeScroll(Gtk::Window *w = nullptr);
extern Gtk::Notebook *makeNotebook();
extern std::pair<int, int> getMonitorSize(Gtk::Window &);
extern std::pair<int, int> getWindowSize(Gtk::Window &);

constexpr int PixelSize = 32;

static inline size_t makeButtons(Gtk::Window *, Gtk::HBox *, size_t count)
{
    return count;
}

template <typename T, typename F, typename... Args>
static inline size_t makeButtons(Gtk::Window *window, Gtk::HBox *box, size_t count, T icon, F func, Args &&...args)
{
    Gtk::Button *button = Gtk::make_managed<Gtk::Button>(icon);
    button->signal_clicked().connect([window, func = std::move(func)]() {
        window->hide();
        func();
        delete window;
    });
    box->pack_start(*button, Gtk::PACK_EXPAND_PADDING);
    return makeButtons(window, box, count + 1, std::forward<Args>(args)...);
}

extern void sizeScrolledWindow(Gtk::ScrolledWindow &);
extern std::pair<int, int> getContentSize(Gtk::Widget &);

template <typename... Args>
static inline Gtk::Window *createWindow(Gtk::WindowType type, const Glib::ustring &title,
                                        std::pair<Gtk::Widget *, Gtk::Widget *> contents, void *ptr, Args &&...args)
{
    Gtk::Window *parent = (Gtk::Window *)ptr;
    Gtk::Window *window = new Gtk::Window(type);
    window->set_modal(true);
    window->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    window->set_title(title);

    Gtk::VBox *vBox = Gtk::make_managed<Gtk::VBox>();
    window->add(*vBox);

    if (contents.first)
    {
        vBox->pack_start(*contents.first, Gtk::PACK_SHRINK, 10);
        Gtk::Separator *separator = Gtk::make_managed<Gtk::Separator>();
        vBox->pack_start(*separator, Gtk::PACK_SHRINK);
    }

    if (dynamic_cast<Gtk::ScrolledWindow *>(contents.second) == nullptr ||
        dynamic_cast<Gtk::Notebook *>(contents.second) == nullptr)
    {
        Gtk::ScrolledWindow *scroll = makeScroll(window);
        scroll->add(*contents.second);
        vBox->pack_start(*scroll, contents.first == nullptr ? Gtk::PACK_EXPAND_WIDGET : Gtk::PACK_EXPAND_PADDING, 10);
    }
    else
    {
        vBox->pack_start(*contents.second,
                         contents.first == nullptr ? Gtk::PACK_EXPAND_WIDGET : Gtk::PACK_EXPAND_PADDING, 10);
    }

    Gtk::Separator *separator = Gtk::make_managed<Gtk::Separator>();
    vBox->pack_start(*separator, Gtk::PACK_SHRINK);

    {
        Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();
        if (makeButtons(window, hBox, 0, std::forward<Args>(args)...) != 0)
        {
            vBox->pack_start(*hBox, Gtk::PACK_SHRINK, 10);
        }
        else
        {
            delete hBox;
        }
    }

    if (parent != nullptr)
    {
        window->set_transient_for(*parent);
        window->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
    }

    window->show_all_children();
    window->show();
    return window;
}

template <typename... Args>
static inline Gtk::Window *createWindow(const Glib::ustring &title, std::pair<Gtk::Widget *, Gtk::Widget *> contents,
                                        void *ptr, Args &&...args)
{
    return createWindow(Gtk::WINDOW_TOPLEVEL, title, contents, ptr, std::forward<Args>(args)...);
}

template <typename F>
static inline Gtk::Window *showMessageBox(MessageBoxType type, const Glib::ustring &title, const Glib::ustring &message,
                                          void *ptr, F &&func)
{
    Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();
    box->set_spacing(10);

    std::pair<Gtk::Widget *, Gtk::Widget *> contents(nullptr, nullptr);
    try
    {
        auto icons = Gtk::IconTheme::get_default();
        contents.first = Gtk::make_managed<Gtk::Image>(icons->load_icon(getIconName(type), PixelSize));
    }
    catch (const std::exception &)
    {
    }

    contents.second = Gtk::make_managed<Gtk::Label>(message);

    return createWindow(title, contents, ptr, Gtk::Stock::OK, func);
}
} // namespace CanForm