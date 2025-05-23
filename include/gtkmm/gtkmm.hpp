#pragma once

#include <canform.hpp>
#include <filesystem>
#include <glibmm/ustring.h>
#include <gtkmm/drawingarea.h>
#include <unordered_set>

namespace CanForm
{
extern Glib::ustring convert(const std::string &s);
extern Glib::ustring convert(std::string_view);
extern std::string_view convert(const Glib::ustring &);

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

    bool spawnEditor() const;
};

class NotebookPage : public Gtk::DrawingArea
{
  private:
    RenderAtoms atoms;
    CanFormRectangle viewRect;

  public:
    Color clearColor;

  private:
    // wxPoint lastMouse;

    static std::pmr::unordered_set<NotebookPage *> pages;

    friend bool CanForm::getCanvasAtoms(std::string_view, RenderAtomsUser &, void *);

    NotebookPage();

    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &) override;

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

    template <typename F> static void forEachPage(F func)
    {
        for (auto page : pages)
        {
            func(*page);
        }
    }
};
} // namespace CanForm