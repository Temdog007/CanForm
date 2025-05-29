#include <gtkmm/dragList.hpp>

namespace CanForm
{
DragList::DragList(const Glib::ustring &u) : Gtk::TreeView(), column(), treeModel(Gtk::ListStore::create(column))
{
    set_model(treeModel);
    append_column(u, column.name);
    enable_model_drag_source();
    enable_model_drag_dest();
}

void DragList::add(const Glib::ustring &text, void *userData)
{
    Gtk::TreeModel::Row row = *(treeModel->append());
    row[column.name] = text;
    row[column.userData] = userData;
}

DragList::Column::Column() : name(), userData()
{
    add(name);
    add(userData);
}

} // namespace CanForm