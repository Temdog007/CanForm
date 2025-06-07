#include <gtkmm/editableSet.hpp>

namespace CanForm
{
EditableSet::EditableSet() : treeModel(), treeView(), buttonBox(), addButton("Add"), removeButton("Remove"), column()
{
    treeModel = Gtk::ListStore::create(column);
    treeView.set_model(treeModel);

    treeView.append_column_editable("Name", column.name);
    pack_start(treeView, Gtk::PACK_EXPAND_WIDGET);

    treeModel->signal_row_changed().connect(sigc::mem_fun(*this, &EditableSet::on_change));

    pack_start(buttonBox, Gtk::PACK_SHRINK);

    addButton.signal_clicked().connect(sigc::mem_fun(*this, &EditableSet::on_add_clicked));
    buttonBox.add(addButton);

    removeButton.signal_clicked().connect(sigc::mem_fun(*this, &EditableSet::on_remove_clicked));
    buttonBox.add(removeButton);
}

void EditableSet::add(const Glib::ustring &string)
{
    auto row = *(treeModel->append());
    row[column.name] = string;
}

void EditableSet::on_add_clicked()
{
    auto row = *(treeModel->append());
    row[column.name] = "";
    m_added.emit("");
}

void EditableSet::on_remove_clicked()
{
    auto selection = treeView.get_selection();
    auto iter = selection->get_selected();
    if (iter)
    {
        m_removed.emit(iter->get_value(column.name));
        treeModel->erase(iter);
    }
}

void EditableSet::on_change(const Gtk::TreeModel::Path &, const Gtk::TreeModel::iterator &)
{
    m_cleared.emit();
    for (const auto &row : treeModel->children())
    {
        m_added.emit(row[column.name]);
    }
}

EditableSet::added EditableSet::signal_added()
{
    return m_added;
}

EditableSet::removed EditableSet::signal_removed()
{
    return m_removed;
}

EditableSet::cleared EditableSet::signal_cleared()
{
    return m_cleared;
}
} // namespace CanForm