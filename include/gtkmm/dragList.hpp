#pragma once

#include <gtkmm.h>

namespace CanForm
{
class DragList : public Gtk::VBox
{
  public:
    struct Button : public Gtk::Button
    {
        void *userData;
        bool hasDrag;

        template <typename... Args>
        Button(void *d, Args &&...args) : Gtk::Button(std::forward<Args>(args)...), userData(d), hasDrag(false)
        {
        }

        virtual ~Button()
        {
        }
    };

  private:
    std::vector<Gtk::TargetEntry> listTargets;
    Gtk::VBox *box;
    Gtk::Label *dragLabel;
    Button *dragTarget;

    void reposition(Gtk::Button *dragOver);

  public:
    DragList();
    virtual ~DragList()
    {
    }

    Button *addButton(const Glib::ustring &, void *userData = nullptr, bool add = true);
};
} // namespace CanForm