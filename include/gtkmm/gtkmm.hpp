#pragma once

#include <canform.hpp>
#include <filesystem>
#include <glibmm/ustring.h>
#include <gtkmm/drawingarea.h>
#include <memory>
#include <unordered_set>

namespace CanForm
{
extern Glib::ustring convert(const std::string &s);
extern Glib::ustring convert(std::string_view);
extern String convert(const Glib::ustring &);
extern std::string_view toView(const Glib::ustring &);

struct IBufferSetter
{
    virtual ~IBufferSetter()
    {
    }
    virtual void set(Glib::ustring &&) = 0;
};

template <typename F> struct BufferSetter : public IBufferSetter
{
    F func;
    BufferSetter(F &&f) : func(std::move(f))
    {
    }
    virtual ~BufferSetter()
    {
    }
    virtual void set(Glib::ustring &&s) override
    {
        func(std::move(s));
    }
};

class TempFile
{
  private:
    Glib::ustring path;
    Glib::ustring extension;

  public:
    TempFile(const Glib::ustring &ext);
    ~TempFile();

    Glib::ustring getName() const;
    std::filesystem::path getPath() const;

    bool read(String &string) const;
    bool write(const String &string) const;

    bool read(Glib::ustring &) const;
    bool write(const Glib::ustring &) const;

    static bool spawnEditor(std::shared_ptr<TempFile>, std::shared_ptr<IBufferSetter> &&, Gtk::Window *);

    template <typename F, std::enable_if_t<std::is_invocable<F, Glib::ustring &&>::value, bool> = true>
    static bool spawnEditor(std::shared_ptr<TempFile> file, F f, Gtk::Window *window)
    {
        std::shared_ptr<BufferSetter<F>> setter = std::make_shared<BufferSetter<F>>(std::move(f));
        return spawnEditor(file, std::move(setter), window);
    }
};

using TimePoint = std::chrono::system_clock::time_point;
static inline TimePoint now() noexcept
{
    return std::chrono::system_clock::now();
}

class NotebookPage : public Gtk::DrawingArea
{
  private:
    RenderAtoms atoms;
    CanFormRectangle viewRect;
    CanFormRectangle viewBounds;
    std::optional<std::pair<gdouble, gdouble>> movePoint;
    std::pair<gdouble, gdouble> lastMouse;

    class Timer
    {
      private:
        sigc::connection connection;

      public:
        Timer(NotebookPage &);
        ~Timer();

        bool is_connected() const;
    };
    std::optional<Timer> timer;

  public:
    Color clearColor;

  private:
    bool Update(int);

    static std::pmr::unordered_set<NotebookPage *> pages;

    friend bool CanForm::getCanvasAtoms(std::string_view, RenderAtomsUser &, void *);

    NotebookPage();

    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &) override;
    virtual bool on_button_press_event(GdkEventButton *) override;
    virtual bool on_button_release_event(GdkEventButton *) override;
    virtual bool on_motion_notify_event(GdkEventMotion *) override;
    virtual bool on_scroll_event(GdkEventScroll *) override;
    virtual bool on_leave_notify_event(GdkEventCrossing *) override;

  public:
    virtual ~NotebookPage();

    static NotebookPage *create();

    RenderAtoms &getAtoms() noexcept
    {
        return atoms;
    }
    const RenderAtoms &getAtoms() const noexcept
    {
        return atoms;
    }

    constexpr CanFormRectangle getViewRect() const noexcept
    {
        return viewRect;
    }
    CanFormRectangle &getViewRect() noexcept
    {
        return viewRect;
    }

    void appendToNotebook(Gtk::Notebook &, std::string_view);

    template <typename F> static void forEachPage(F func)
    {
        for (auto page : pages)
        {
            func(*page);
        }
    }

    void redraw();
};
} // namespace CanForm