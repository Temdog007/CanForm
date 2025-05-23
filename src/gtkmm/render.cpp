#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>

namespace CanForm
{
std::pmr::unordered_set<NotebookPage *> NotebookPage::pages;

NotebookPage::NotebookPage() : Gtk::DrawingArea(), atoms(), viewRect(), clearColor()
{
    clearColor.red = 0.5;
    clearColor.green = 0.5;
    clearColor.blue = 0.5;
    clearColor.alpha = 1.0;
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

constexpr double degrees = M_PI / 180.0;

struct Drawer
{
    const Cairo::RefPtr<Cairo::Context> &ctx;

    constexpr Drawer(const Cairo::RefPtr<Cairo::Context> &c) noexcept : ctx(c)
    {
    }

    bool operator()(const CanFormRectangle &r)
    {
        ctx->rectangle(r.x, r.y, r.w, r.h);
        return true;
    }
    bool operator()(const RoundedRectangle &rr)
    {
        ctx->begin_new_sub_path();
        const CanFormRectangle &r = rr.rectangle;
        ctx->arc(r.x + r.w - rr.radius, r.y + rr.radius, rr.radius, -90 * degrees, 0 * degrees);
        ctx->arc(r.x + r.w - rr.radius, r.y + r.h - rr.radius, rr.radius, 0 * degrees, 90 * degrees);
        ctx->arc(r.x + rr.radius, r.y + r.h - rr.radius, rr.radius, 90 * degrees, 180 * degrees);
        ctx->arc(r.x + rr.radius, r.y + rr.radius, rr.radius, 180 * degrees, 270 * degrees);
        ctx->close_path();
        return true;
    }
    bool operator()(const Ellipse &r)
    {
        ctx->translate(r.x, r.y);
        ctx->scale(r.w, r.h);
        ctx->translate(-r.x, -r.y);
        ctx->begin_new_path();
        ctx->arc(r.x, r.y, 0.5, 0, 2 * M_PI);
        ctx->close_path();
        return true;
    }
    bool operator()(const Text &t)
    {
        ctx->move_to(t.x, t.y);
        ctx->show_text(std::string(t.string));
        return false;
    }
    void operator()(const RenderAtom &atom)
    {
        const RenderStyle &style = atom.style;
        ctx->save();
        ctx->set_source_rgba(style.color.red, style.color.green, style.color.blue, style.color.alpha);
        if (std::visit(*this, atom.renderType))
        {
            if (style.fill)
            {
                ctx->fill();
            }
            else
            {
                ctx->stroke();
            }
        }
        ctx->restore();
    }
};

static Cairo::RefPtr<Cairo::Context> cairoContext;
bool NotebookPage::on_draw(const Cairo::RefPtr<Cairo::Context> &ctx)
{
    cairoContext = ctx;
    ctx->set_source_rgba(clearColor.red, clearColor.green, clearColor.blue, clearColor.alpha);
    ctx->paint();

    Gtk::Allocation allocation = get_allocation();
    double width = allocation.get_width();
    double height = allocation.get_height();

    Cairo::Matrix matrix = Cairo::identity_matrix();
    const auto [cx, cy] = viewRect.center();

    matrix.translate(width * 0.5, height * 0.5);
    matrix.scale(viewRect.w / width, viewRect.h / height);
    matrix.translate(width * -0.5, height * -0.5);

    matrix.translate(cx - width * 0.5, cy - height * 0.5);

    ctx->set_matrix(matrix);

    Drawer drawer(ctx);
    for (const auto &atom : atoms)
    {
        drawer(atom);
    }
    return true;
}

Rectangle getTextBounds(std::string_view s) noexcept
{
    Rectangle r;
    if (cairoContext)
    {
        Cairo::TextExtents extents;
        cairoContext->get_text_extents(std::string(s), extents);
        r.w = extents.width;
        r.h = extents.height;
    }
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