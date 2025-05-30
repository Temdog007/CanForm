#pragma once

#include <gtkmm.h>

namespace CanForm
{
class DragList : public Gtk::TreeView
{
  private:
    struct Column : public Gtk::TreeModel::ColumnRecord
    {
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<void *> userData;
        Column();
        virtual ~Column()
        {
        }
    };

    Column column;
    Glib::RefPtr<Gtk::ListStore> treeModel;

  protected:
    virtual void on_drag_end(const Glib::RefPtr<Gdk::DragContext> &) override;

  public:
    DragList(const Glib::ustring &);
    virtual ~DragList()
    {
    }

    void add(const Glib::ustring &, void *userData = nullptr);

    using row_reorder = sigc::signal<void(const Glib::ustring &, size_t, void *)>;
    row_reorder signal_row_reorder();

  protected:
    row_reorder m_row_reorder;
};
} // namespace CanForm