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

    auto box = Gtk::make_managed<Gtk::VBox>();
    box->set_spacing(10);

    auto over = Gtk::make_managed<Gtk::ComboBoxText>();
    for (auto &[name, _] : variant.map)
    {
        over->append(convert(name));
    }
    over->set_active_text(convert(variant.selected));
    box->pack_start(*over, Gtk::PACK_SHRINK);

    auto under = Gtk::make_managed<Gtk::VBox>();
    under->set_margin_bottom(10);
    box->pack_start(*under, Gtk::PACK_EXPAND_WIDGET);

    using Map = std::pmr::map<Glib::ustring, Gtk::Widget *>;
    auto map = std::make_shared<Map>();

    const auto activate = [map, &variant, over, under]() {
        const auto text = over->get_active_text();
        variant.selected = convert(text);
        auto iter = map->find(text);
        auto iter2 = variant.map.find(variant.selected);
        if (iter == map->end() && iter2 != variant.map.end())
        {
            Gtk::Widget *widget = std::visit(FormVisitor(), *iter2->second);
            under->pack_start(*widget, Gtk::PACK_SHRINK);
            under->show_all_children();
            map->emplace(text, widget);
        }
        for (auto &[name, widget] : *map)
        {
            widget->set_visible(name == text);
        }
    };
    over->signal_changed().connect(activate);
    activate();

    frame->add(*box);
    return frame;
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

    auto box = Gtk::make_managed<Gtk::HBox>();
    box->set_spacing(10);

    auto left = Gtk::make_managed<Gtk::VBox>();
    box->pack_start(*left, Gtk::PACK_SHRINK);

    auto right = Gtk::make_managed<Gtk::VBox>();
    box->pack_start(*right, Gtk::PACK_EXPAND_WIDGET);

    using Map = std::pmr::map<String, Gtk::Widget *>;
    auto map = std::make_shared<Map>();

    const auto activate = [map, right, &enableForm](const String &key, bool visible) {
        auto iter = enableForm->find(key);
        if (iter == enableForm->end())
        {
            return;
        }
        iter->second.first = visible;
        auto iter2 = map->find(key);
        if (iter2 == map->end())
        {
            FormVisitor visitor;
            visitor.name = key;
            auto widget = std::visit(visitor, *iter->second.second);
            widget->set_visible(visible);
            right->pack_start(*widget, Gtk::PACK_SHRINK);
            right->show_all_children();
            map->emplace(key, widget);
        }
        else
        {
            iter2->second->set_visible(visible);
        }
    };

    for (auto &pair : enableForm.map)
    {
        auto button = Gtk::make_managed<Gtk::CheckButton>(convert(pair.first));
        button->set_active(pair.second.first);
        if (pair.second.first)
        {
            activate(pair.first, true);
        }
        button->signal_toggled().connect(
            [button, key = pair.first, activate]() { activate(key, button->get_active()); });
        left->pack_start(*button, Gtk::PACK_SHRINK);
    }

    if (expander == nullptr)
    {
        return box;
    }
    expander->add(*box);
    return expander;
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

void FormExecute::execute(std::string_view title, const std::shared_ptr<FormExecute> &formExecute, void *ptr)
{
    createWindow(
        convert(title), std::make_pair(nullptr, std::visit(FormVisitor(), *(formExecute->form))), ptr, Gtk::Stock::OK,
        [formExecute]() { formExecute->ok(); }, Gtk::Stock::CANCEL, [formExecute]() { formExecute->cancel(); });
}

} // namespace CanForm