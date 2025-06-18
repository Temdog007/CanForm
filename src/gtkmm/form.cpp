#include <gtkmm/editableSet.hpp>
#include <gtkmm/form_visitor.hpp>

namespace CanForm
{
Gtk::Frame *FormVisitor::makeFrame() const
{
    return Gtk::make_managed<Gtk::Frame>(convert(name));
}

Gtk::Widget *FormVisitor::operator()(std::monostate &)
{
    return makeFrame();
}
Gtk::Widget *FormVisitor::operator()(bool &b)
{
    Gtk::CheckButton *button = Gtk::make_managed<Gtk::CheckButton>(convert(name));
    button->set_active(b);
    button->signal_toggled().connect([&b, button]() { b = button->get_active(); });
    return button;
}

Gtk::TextView *FormVisitor::makeTextView()
{
    Gtk::TextView *entry = Gtk::make_managed<Gtk::TextView>();
    entry->set_pixels_above_lines(5);
    entry->set_pixels_below_lines(5);
    entry->set_bottom_margin(10);
    return entry;
}

Gtk::Widget *FormVisitor::operator()(String &s)
{
    auto frame = makeFrame();
    Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();

    Gtk::TextView *entry = makeTextView();

    auto buffer = entry->get_buffer();
    buffer->set_text(convert(s));
    buffer->signal_changed().connect([&s, buffer]() { s = convert(buffer->get_text()); });

    box->pack_start(*entry, Gtk::PACK_EXPAND_WIDGET, 10);

    addSyncFile(*box, buffer);

    frame->add(*box);
    return frame;
}

Gtk::Widget *FormVisitor::operator()(ComplexString &s)
{
    auto frame = makeFrame();
    Gtk::VBox *box = Gtk::make_managed<Gtk::VBox>();

    Gtk::TextView *entry = makeTextView();

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

Gtk::Widget *FormVisitor::operator()(RangedValue &n)
{
    return std::visit(*this, n);
}

Gtk::Widget *FormVisitor::operator()(StringSet &set)
{
    auto frame = makeFrame();

    auto editableSet = Gtk::make_managed<EditableSet>();
    for (auto &text : set)
    {
        editableSet->add(convert(text));
    }

    editableSet->signal_added().connect([&set](const Glib::ustring &s) { set.emplace(convert(s)); });
    editableSet->signal_removed().connect([&set](const Glib::ustring &s) {
        std::string string(s);
        set.erase(String(string));
    });
    editableSet->signal_cleared().connect([&set]() { set.clear(); });

    frame->add(*editableSet);

    return frame;
}

Gtk::Widget *FormVisitor::operator()(StringSelection &selection)
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

Gtk::Widget *FormVisitor::operator()(StringMap &map)
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

Gtk::Widget *FormVisitor::operator()(VariantForm &variant)
{
    auto frame = makeFrame();
    auto notebook = makeNotebook();
    frame->add(*notebook);
    notebook->signal_switch_page().connect([notebook, &variant](Gtk::Widget *widget, guint) {
        if (notebook->is_visible())
        {
            variant.selected = convert(notebook->get_tab_label_text(*widget));
            auto scroll = get<Gtk::ScrolledWindow>(widget);
            if (scroll != nullptr)
            {
                const auto allocation = widget->get_allocation();
                scroll->set_min_content_width(allocation.get_width());
                scroll->set_min_content_height(allocation.get_height());
            }
        }
    });

    auto iter = variant.map.begin();
    auto func = [iter, notebook, &variant]() mutable {
        if (!notebook->is_visible() || iter == variant.map.end())
        {
            return false;
        }
        auto &n = iter->first;
        auto &form = iter->second;
        FormVisitor visitor;
        notebook->append_page(*std::visit(visitor, *form), convert(n));
        notebook->show_all_children();
        if (variant.selected == n)
        {
            notebook->set_current_page(notebook->get_n_pages() - 1);
        }
        ++iter;
        return true;
    };
    func();
    Glib::signal_idle().connect(func);
    return frame;
}

Gtk::Widget *FormVisitor::operator()(StructForm &structForm)
{
    Gtk::Expander *expander = nullptr;
    if (!name.empty())
    {
        expander = Gtk::make_managed<Gtk::Expander>();
        expander->set_label(convert(name));
        expander->set_label_fill(true);
        expander->set_resize_toplevel(true);
        expander->set_expanded(true);
    }

    Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    size_t index = 0;
    for (auto &[n, form] : *structForm)
    {
        const int row = index / structForm.columns;
        const int column = index % structForm.columns;
        name = n;
        grid->attach(*std::visit(*this, *form), column, row);
        ++index;
    }

    if (expander == nullptr)
    {
        return grid;
    }
    expander->add(*grid);
    return expander;
}

Gtk::Widget *FormVisitor::operator()(EnableForm &enableForm)
{
    Gtk::Expander *expander = nullptr;
    if (!name.empty())
    {
        expander = Gtk::make_managed<Gtk::Expander>();
        expander->set_label(convert(name));
        expander->set_label_fill(true);
        expander->set_resize_toplevel(true);
        expander->set_expanded(true);
    }
    name = std::string_view();

    Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    size_t index = 0;
    for (auto &[n, pair] : *enableForm)
    {
        bool &enabled = pair.first;
        Form &inner = pair.second;

        Gtk::Widget *widget = std::visit(*this, *inner);
        widget->signal_show().connect([widget, &enabled]() { widget->set_visible(enabled); });

        Gtk::CheckButton *button = Gtk::make_managed<Gtk::CheckButton>(convert(n));
        button->set_active(enabled);
        button->signal_toggled().connect([&enabled, widget, button]() {
            enabled = button->get_active();
            widget->set_visible(enabled);
        });

        grid->attach(*button, 0, index, 1, 1);
        grid->attach(*widget, 1, index, 3, 1);
        ++index;
    }

    if (expander == nullptr)
    {
        return grid;
    }
    expander->add(*grid);
    return expander;
}

void FormExecute::execute(std::string_view title, const std::shared_ptr<FormExecute> &formExecute, void *ptr)
{
    createWindow(
        convert(title), std::make_pair(nullptr, std::visit(FormVisitor(), *(formExecute->form))), ptr, Gtk::Stock::OK,
        [formExecute]() { formExecute->ok(); }, Gtk::Stock::CANCEL, [formExecute]() { formExecute->cancel(); });
}

} // namespace CanForm