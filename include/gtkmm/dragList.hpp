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

  public:
    DragList(const Glib::ustring &);
    virtual ~DragList()
    {
    }

    void add(const Glib::ustring &, void *userData = nullptr);
};
} // namespace CanForm