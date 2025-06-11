#pragma once

#include <gtkmm/gtkmm.hpp>
#include <gtkmm/window.hpp>

namespace CanForm
{
class FormVisitor
{
  private:
    std::string_view name;

    Gtk::Frame *makeFrame() const;

    template <typename B> void addSyncFile(Gtk::Box &box, B buffer) const;

  public:
    Gtk::Widget *operator()(std::monostate &);
    Gtk::Widget *operator()(bool &);

    Gtk::Widget *operator()(String &);
    Gtk::Widget *operator()(ComplexString &);

    template <typename T> Gtk::Widget *operator()(Range<T> &);
    Gtk::Widget *operator()(RangedValue &);

    Gtk::Widget *operator()(SortableList &);
    Gtk::Widget *operator()(StringSet &);
    Gtk::Widget *operator()(StringSelection &);
    Gtk::Widget *operator()(StringMap &);

    Gtk::Widget *operator()(VariantForm &);
    Gtk::Widget *operator()(StructForm &);

    Gtk::TextView *makeTextView();
};

template <typename T> Gtk::Widget *FormVisitor::operator()(Range<T> &value)
{
    auto frame = makeFrame();
    Gtk::SpinButton *button = Gtk::make_managed<Gtk::SpinButton>();

    const auto [min, max] = value.getMinMax();
    button->set_range(min, max);
    if constexpr (std::is_floating_point_v<T>)
    {
        button->set_digits(6);
    }
    button->set_value(*value);

    button->set_increments(1, 10);

    button->signal_value_changed().connect([button, &value]() { value = static_cast<T>(button->get_value()); });

    frame->add(*button);
    return frame;
}

template <typename B> void FormVisitor::addSyncFile(Gtk::Box &box, B buffer) const
{
    Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();

    SyncButton *button = Gtk::make_managed<SyncButton>(convert(name), buffer);
    hBox->add(*button);

    box.pack_start(*hBox, Gtk::PACK_SHRINK);
}
} // namespace CanForm