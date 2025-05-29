#include <gtkmm/dragList.hpp>

namespace CanForm
{
DragList::DragList()
    : Gtk::VBox(), listTargets(), box(Gtk::make_managed<Gtk::VBox>()), dragLabel(Gtk::make_managed<Gtk::Label>()),
      dragTarget(nullptr)
{
    listTargets.push_back(Gtk::TargetEntry("STRING"));
    listTargets.push_back(Gtk::TargetEntry("text/plain"));

    pack_start(*box, Gtk::PACK_EXPAND_WIDGET);
    pack_start(*dragLabel, Gtk::PACK_SHRINK);
}

DragList::Button *DragList::addButton(const Glib::ustring &text, void *userData, bool add)
{
    Button *button = Gtk::make_managed<Button>(userData, text);
    button->drag_source_set(listTargets);
    button->drag_dest_set(listTargets);
    button->signal_drag_begin().connect([this, button](const Glib::RefPtr<Gdk::DragContext> &) {
        dragLabel->set_label(Glib::ustring::sprintf("Dragging %s", button->get_label()));
        dragTarget = button;
    });
    button->signal_drag_end().connect([this, button](const Glib::RefPtr<Gdk::DragContext> &) {
        dragLabel->set_label("");
        dragTarget = nullptr;
    });
    button->signal_drag_motion().connect([this, button](const Glib::RefPtr<Gdk::DragContext> &, int, int, guint) {
        if (dragTarget == button)
        {
            return false;
        }
        if (!button->hasDrag)
        {
            button->hasDrag = true;
            reposition(button);
        }
        return true;
    });
    button->signal_drag_leave().connect(
        [this, button](const Glib::RefPtr<Gdk::DragContext> &, guint) { button->hasDrag = false; });
    if (add)
    {
        box->pack_start(*button);
    }
    return button;
}

void DragList::reposition(Gtk::Button *dragOver)
{
    Gtk::VBox tempBox;
    box->foreach ([this, &tempBox, dragOver](Gtk::Widget &widget) {
        Button &button = dynamic_cast<Button &>(widget);
        if (dragTarget == &button)
        {
            return;
        }
        if (dragOver == &button && dragTarget != nullptr)
        {
            Button *b = addButton(dragTarget->get_label(), dragTarget->userData, false);
            tempBox.pack_start(*b);
        }
        Button *b = addButton(button.get_label(), button.userData, false);
        tempBox.pack_start(*b);
    });
    *box = std::move(tempBox);
    box->show_all_children();
    box->queue_draw();
    puts("Repositioned");
}

} // namespace CanForm