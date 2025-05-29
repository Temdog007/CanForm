#pragma once

#include <gtkmm.h>

namespace CanForm
{
extern Gtk::ScrolledWindow *makeScroll(Gtk::Window *);
extern std::pair<int, int> getSize(Gtk::Container &);
extern std::pair<int, int> getMonitorSize(Gtk::Window &);
extern std::pair<int, int> getWindowSize(Gtk::Window &);

constexpr int PixelSize = 32;

static inline void makeButtons(Gtk::Window *, Gtk::HBox *)
{
}

template <typename T, typename F, typename... Args>
static inline void makeButtons(Gtk::Window *window, Gtk::HBox *box, T icon, F func, Args &&...args)
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

template <typename... Args>
static inline Gtk::Window *createWindow(Gtk::WindowType type, std::string_view title, Gtk::Widget *content, void *ptr,
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
static inline Gtk::Window *createWindow(std::string_view title, Gtk::Widget *content, void *ptr, Args &&...args)
{
    return createWindow(Gtk::WINDOW_TOPLEVEL, title, content, ptr, std::forward<Args>(args)...);
}

} // namespace CanForm