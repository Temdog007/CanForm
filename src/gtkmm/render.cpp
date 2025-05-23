#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>

namespace CanForm
{
std::pmr::unordered_set<NotebookPage *> NotebookPage::pages;

NotebookPage::NotebookPage() : Gtk::DrawingArea(), atoms(), viewRect()
{
}

NotebookPage::~NotebookPage()
{
    pages.erase(this);
}

NotebookPage *NotebookPage::create()
{
    NotebookPage *page = Gtk::manage(new NotebookPage());
    pages.insert(page);
    return page;
}

Rectangle getTextBounds(std::string_view s) noexcept
{
    Rectangle r;
    return r;
}

bool getCanvasAtoms(std::string_view canvas, RenderAtomsUser &users, void *ptr)
{
    bool found = false;
    const Glib::ustring target = convert(canvas);
    NotebookPage::forEachPage([&](NotebookPage &page) {
        auto parent = page.get_parent();
        if (auto book = dynamic_cast<Gtk::Notebook *>(parent))
        {
            const size_t count = book->get_n_pages();
            for (size_t i = 0; i < count; ++i)
            {
                Gtk::Widget *widget = book->get_nth_page(i);
                if (widget != nullptr && target == book->get_tab_label_text(*widget))
                {
                    users.use(&page, page.atoms, page.viewRect);
                    found = true;
                    return;
                }
            }
        }
        if (auto window = dynamic_cast<Gtk::Window *>(parent))
        {
            if (window->get_title() == target)
            {
                users.use(&page, page.atoms, page.viewRect);
                found = true;
                return;
            }
        }
    });
    if (found)
    {
        return true;
    }
    Gtk::Widget *parent = (Gtk::Widget *)ptr;
    if (auto book = dynamic_cast<Gtk::Notebook *>(parent))
    {
        NotebookPage *page = NotebookPage::create();
        book->append_page(*page, convert(canvas));
        users.use(page, page->atoms, page->viewRect);
        book->show_all_children();
        return true;
    }

    return false;
}

} // namespace CanForm