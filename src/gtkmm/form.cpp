#include <gtkmm/gtkmm.hpp>
#include <gtkmm/window.hpp>

namespace CanForm
{
class FormVisitor
{
  private:
    std::string_view name;
    size_t columns;

    Gtk::Frame *makeFrame() const
    {
        return Gtk::make_managed<Gtk::Frame>(convert(name));
    }

    template <typename B> void addSyncFile(Gtk::Box &box, B buffer) const
    {
        Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();

        SyncButton *button = Gtk::make_managed<SyncButton>(convert(name), buffer);
        hBox->add(*button);

        box.pack_start(*hBox, Gtk::PACK_SHRINK);
    }

  public:
    constexpr FormVisitor(size_t c) noexcept : name(), columns(c)
    {
    }

    Gtk::Widget *operator()(bool &b)
    {
        Gtk::CheckButton *button = Gtk::make_managed<Gtk::CheckButton>(convert(name));
        button->set_active(b);
        button->signal_clicked().connect([&b, button]() { b = button->get_active(); });
        return button;
    }

    Gtk::Widget *operator()(String &s)
    {
        auto frame = makeFrame();
        Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();

        Gtk::Entry *entry = Gtk::make_managed<Gtk::Entry>();
        auto buffer = entry->get_buffer();
        const auto updateText = [&s, buffer]() { s = convert(buffer->get_text()); };

        buffer->set_text(convert(s));
        buffer->signal_inserted_text().connect([updateText](guint, const char *, guint) { updateText(); });
        buffer->signal_deleted_text().connect([updateText](guint, guint) { updateText(); });

        box->pack_start(*entry, Gtk::PACK_EXPAND_PADDING, 10);

        addSyncFile(*box, buffer);

        frame->add(*box);
        return frame;
    }

    Gtk::Widget *operator()(ComplexString &s)
    {
        auto frame = makeFrame();
        Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();

        Gtk::TextView *entry = Gtk::make_managed<Gtk::TextView>();
        entry->set_pixels_above_lines(5);
        entry->set_pixels_below_lines(5);
        entry->set_bottom_margin(10);

        auto buffer = entry->get_buffer();
        buffer->set_text(convert(s.string));
        buffer->signal_changed().connect([&s, buffer]() { s.string = convert(buffer->get_text()); });

        box->pack_start(*entry, Gtk::PACK_EXPAND_WIDGET, 10);

        addSyncFile(*box, buffer);

        Gtk::Expander *expander = Gtk::make_managed<Gtk::Expander>();
        expander->set_label("Text Insertions");
        expander->set_label_fill(true);
        expander->set_resize_toplevel(true);

        Gtk::VBox *box2 = Gtk::make_managed<Gtk::VBox>();
        for (auto &[label, set] : s.map)
        {
            Gtk::Frame *frame = Gtk::make_managed<Gtk::Frame>(convert(label));
            Gtk::FlowBox *flow = Gtk::make_managed<Gtk::FlowBox>();
            for (auto &s : set)
            {
                Gtk::Button *button = Gtk::make_managed<Gtk::Button>(convert(s));
                button->signal_clicked().connect([buffer, entry, &s]() {
                    buffer->insert_at_cursor(s.data(), s.data() + s.size());
                    entry->grab_focus();
                });
                flow->add(*button);
            }
            frame->add(*flow);
            box2->add(*frame);
        }

        expander->add(*box2);
        box->pack_start(*expander, Gtk::PACK_EXPAND_PADDING);

        frame->add(*box);
        return frame;
    }

    template <typename T> Gtk::Widget *operator()(Range<T> &value)
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

    Gtk::Widget *operator()(RangedValue &n)
    {
        return std::visit(*this, n);
    }

    enum class RepositionMode
    {
        Swap,
        MoveBefore,
        MoveAfter,
    };

    template <typename T> static void reposition(T &list, size_t oldPosition, size_t newPosition)
    {
        auto iter = list.begin() + oldPosition;
        auto item = std::move(*iter);
        list.erase(iter);
        if (newPosition < list.size())
        {
            list.emplace(list.begin() + newPosition, std::move(item));
        }
        else
        {
            list.emplace_back(std::move(item));
        }
    }

    static void setStringList(Gtk::VBox *box, std::shared_ptr<RepositionMode> mode, StringList &list)
    {
        box->foreach ([box](Gtk::Widget &widget) { box->remove(widget); });
        using Pair = std::optional<size_t>;
        std::shared_ptr<Pair> ptr = std::make_shared<Pair>();
        for (size_t i = 0; i < list.size(); ++i)
        {
            Gtk::ToggleButton *button = Gtk::make_managed<Gtk::ToggleButton>(convert(list[i].first));
            button->signal_toggled().connect([&list, button, box, mode, ptr, i]() {
                Pair &pair = *ptr;
                if (!button->get_active())
                {
                    if (pair == i)
                    {
                        pair.reset();
                        return;
                    }
                }
                if (!pair)
                {
                    pair.emplace(i);
                    return;
                }
                if (*pair == i)
                {
                    return;
                }
                switch (*mode)
                {
                case RepositionMode::MoveBefore:
                    reposition(list, *pair, i);
                    break;
                case RepositionMode::MoveAfter:
                    reposition(list, *pair, i + 1);
                    break;
                default:
                    std::swap(list[*pair], list[i]);
                    break;
                }
                setStringList(box, mode, list);
            });
            box->add(*button);
        }
        box->show_all_children();
    }

    Gtk::Widget *operator()(StringList &list)
    {
        auto frame = makeFrame();
        Gtk::VBox *vBox = Gtk::make_managed<Gtk::VBox>();

        Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();
        vBox->add(*hBox);

        Gtk::RadioButtonGroup group;

        std::shared_ptr<RepositionMode> mode = std::make_shared<RepositionMode>();

        Gtk::RadioButton *button = Gtk::make_managed<Gtk::RadioButton>(group, "Swap");
        button->signal_clicked().connect([mode]() { *mode = RepositionMode::Swap; });
        hBox->add(*button);

        button = Gtk::make_managed<Gtk::RadioButton>(group, "Move Before");
        button->signal_clicked().connect([mode]() { *mode = RepositionMode::MoveBefore; });
        hBox->add(*button);

        button = Gtk::make_managed<Gtk::RadioButton>(group, "Move After");
        button->signal_clicked().connect([mode]() { *mode = RepositionMode::MoveAfter; });
        hBox->add(*button);

        Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();
        setStringList(box, mode, list);
        vBox->add(*box);

        frame->add(*vBox);
        return frame;
    }

    Gtk::Widget *operator()(StringSelection &selection)
    {
        auto frame = makeFrame();
        Gtk::ComboBoxText *box = Gtk::make_managed<Gtk::ComboBoxText>();
        for (auto &text : selection.set)
        {
            box->append(convert(text));
        }
        auto s = selection.getSelection();
        if (s)
        {
            box->set_active_text(convert(*s));
        }
        box->signal_changed().connect([box, &selection]() { selection.setSelection(convert(box->get_active_text())); });
        frame->add(*box);
        return frame;
    }

    Gtk::Widget *operator()(StringMap &map)
    {
        auto frame = makeFrame();
        Gtk::FlowBox *box = Gtk::make_managed<Gtk::FlowBox>();
        for (auto &pair : map)
        {
            Gtk::CheckButton *button = Gtk::make_managed<Gtk::CheckButton>(convert(pair.first));
            button->set_active(pair.second);
            button->signal_clicked().connect([&pair, button]() { pair.second = button->get_active(); });
            box->add(*button);
        }
        frame->add(*box);
        return frame;
    }

    Gtk::Widget *operator()(MultiForm &multi)
    {
        auto frame = makeFrame();
        Gtk::Notebook *notebook = Gtk::make_managed<Gtk::Notebook>();
        notebook->set_scrollable(true);
        frame->add(*notebook);
        size_t selected = 0;
        size_t current = 0;
        for (auto &[n, form] : multi.tabs)
        {
            notebook->append_page(*operator()(form), convert(n));
            if (multi.selected == n)
            {
                selected = current;
            }
            ++current;
        }
        notebook->show_all_children();
        notebook->set_current_page(selected);
        notebook->signal_switch_page().connect([notebook, &multi](Gtk::Widget *widget, guint) {
            if (notebook->is_visible())
            {
                multi.selected = convert(notebook->get_tab_label_text(*widget));
            }
        });
        return frame;
    }

    Gtk::Widget *operator()(Form &form)
    {
        const int rows = std::max(static_cast<size_t>(1), form.datas.size() / columns);
        Gtk::Table *table = Gtk::make_managed<Gtk::Table>(rows, columns);

        size_t index = 0;
        const Gtk::AttachOptions options = Gtk::SHRINK;
        for (auto &[n, data] : form.datas)
        {
            const int row = index / columns;
            const int column = index % columns;
            name = n;
            table->attach(*std::visit(*this, *data), column, column + 1, row, row + 1, options, options, 10, 10);
            ++index;
        }
        return table;
    }
};

void FormExecute::execute(std::string_view title, const std::shared_ptr<FormExecute> &formExecute, size_t columns,
                          void *ptr)
{
    FormVisitor visitor(columns);
    createWindow(
        title, visitor(formExecute->form), ptr, Gtk::Stock::OK, [formExecute]() { formExecute->ok(); },
        Gtk::Stock::CANCEL, [formExecute]() { formExecute->cancel(); });
}
} // namespace CanForm