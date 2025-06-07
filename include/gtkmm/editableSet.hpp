#pragma once

#include <gtkmm.h>

namespace CanForm
{
class EditableSet : public Gtk::VBox
{
  private:
    Glib::RefPtr<Gtk::ListStore> treeModel;
    Gtk::TreeView treeView;
    Gtk::VBox vBox;
    Gtk::ButtonBox buttonBox;
    Gtk::Button addButton, removeButton;

    struct Column : public Gtk::TreeModel::ColumnRecord
    {
        Gtk::TreeModelColumn<Glib::ustring> name;
        Column() : name()
        {
            add(name);
        }
        virtual ~Column()
        {
        }
    };
    Column column;

    void on_add_clicked();
    void on_remove_clicked();
    void on_change(const Gtk::TreeModel::Path &, const Gtk::TreeModel::iterator &);

  public:
    EditableSet();
    virtual ~EditableSet()
    {
    }

    void add(const Glib::ustring &);

    using added = sigc::signal<void(const Glib::ustring &)>;
    added signal_added();

    using removed = sigc::signal<void(const Glib::ustring &)>;
    removed signal_removed();

    using cleared = sigc::signal<void()>;
    cleared signal_cleared();

  protected:
    added m_added;
    removed m_removed;
    cleared m_cleared;
};
} // namespace CanForm