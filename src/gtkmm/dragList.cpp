#include <gtkmm/dragList.hpp>

namespace CanForm
{
DragList::DragList(const Glib::ustring &u)
    : Gtk::TreeView(), column(), treeModel(Gtk::ListStore::create(column)), m_row_reorder()
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

void DragList::on_drag_end(const Glib::RefPtr<Gdk::DragContext> &)
{
    size_t index = 0;
    for (const Gtk::TreeRow &row : treeModel->children())
    {
        m_row_reorder.emit(row[column.name], index, row[column.userData]);
        ++index;
    }
}

DragList::row_reorder DragList::signal_row_reorder()
{
    return m_row_reorder;
}

DragList::Column::Column() : name(), userData()
{
    add(name);
    add(userData);
}

} // namespace CanForm