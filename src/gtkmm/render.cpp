#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>

namespace CanForm
{
std::pmr::unordered_set<NotebookPage *> NotebookPage::pages;

NotebookPage::NotebookPage()
    : Gtk::DrawingArea(), atoms(), viewRect(), movePoint(std::nullopt), lastMouse(0, 0), lastUpdate(now()),
      timer(std::nullopt), clearColor(), delta(0.0)
{
    clearColor.red = 0.5;
    clearColor.green = 0.5;
    clearColor.blue = 0.5;
    clearColor.alpha = 1.0;
    add_events(Gdk::POINTER_MOTION_MASK);
    add_events(Gdk::BUTTON_PRESS_MASK);
    add_events(Gdk::SMOOTH_SCROLL_MASK);
    add_events(Gdk::LEAVE_NOTIFY_MASK);
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

constexpr double clampIfSmall(double d) noexcept
{
    if (std::abs(d - 0.01) < 0.0)
    {
        return 0.0;
    }
    return d;
}

bool NotebookPage::Update(int)
{
    const auto current = now();
    bool again = false;
    if (movePoint)
    {
        const auto &point = *movePoint;
        delta += std::chrono::duration<double>(current - lastUpdate).count();
        if (delta > 0.01)
        {
            const double diff = std::min(0.1, delta);
            viewRect.x += clampIfSmall((point.first - lastMouse.first)) * diff;
            viewRect.y += clampIfSmall((point.second - lastMouse.second)) * diff;
            delta -= diff;
            queue_draw();
        }
        again = true;
    }
    else
    {
        queue_draw();
    }
    lastUpdate = current;
    return again;
}

NotebookPage::Timer::Timer(NotebookPage &page) : connection()
{
    sigc::slot<bool> slot = sigc::bind(sigc::mem_fun(page, &NotebookPage::Update), 0);
    connection = Glib::signal_timeout().connect(slot, 1);
}

NotebookPage::Timer::~Timer()
{
    connection.disconnect();
}

bool NotebookPage::Timer::is_connected() const
{
    return connection.connected();
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

constexpr double angleBetween(double x1, double y1, double x2, double y2) noexcept
{
    const double dot = x1 * x2 + y1 * y2;
    const double det = x1 * y2 - y1 * x2;
    return std::atan2(det, dot);
}

static Cairo::RefPtr<Cairo::Context> cairoContext;
bool NotebookPage::on_draw(const Cairo::RefPtr<Cairo::Context> &ctx)
{
    cairoContext = ctx;
    ctx->set_source_rgba(clearColor.red, clearColor.green, clearColor.blue, clearColor.alpha);
    ctx->paint();

    Gtk::Allocation allocation = get_allocation();
    const double x = allocation.get_x();
    const double y = allocation.get_y();
    const double width = allocation.get_width();
    const double height = allocation.get_height();

    const auto [cx, cy] = viewRect.center();

    ctx->set_identity_matrix();
    ctx->translate(width * 0.5, height * 0.5);
    ctx->scale(width / viewRect.w, height / viewRect.h);
    ctx->translate(width * -0.5, height * -0.5);
    ctx->translate(cx - width * 0.5, cy - height * 0.5);

    Drawer drawer(ctx);
    for (const auto &atom : atoms)
    {
        drawer(atom);
    }
    if (movePoint)
    {
        ctx->set_identity_matrix();
        ctx->translate(x + movePoint->first, y + movePoint->second);

        const double angle =
            angleBetween(1, 0, lastMouse.first - movePoint->first, lastMouse.second - movePoint->second);
        ctx->rotate(angle);

        ctx->set_source_rgb(1.0, 1.0, 1.0);
        ctx->begin_new_path();
        ctx->arc(0, 0, 20.0, 0, M_PI * 2.0);
        ctx->fill_preserve();
        ctx->set_source_rgb(0.0, 0.0, 0.0);
        ctx->stroke();

        ctx->begin_new_path();
        ctx->move_to(-10, -10);
        ctx->line_to(-10, 10);
        ctx->line_to(15, 0);
        ctx->fill();
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

bool NotebookPage::on_button_press_event(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS)
    {
        switch (event->button)
        {
        case 2:
            if (movePoint)
            {
                movePoint.reset();
            }
            else
            {
                movePoint.emplace(event->x, event->y);
                delta = 0.0;
            }
            redraw();
            break;
        case 3: {
            Gtk::Allocation allocation = get_allocation();
            viewRect.x = 0;
            viewRect.y = 0;
            viewRect.w = allocation.get_width();
            viewRect.h = allocation.get_height();
            movePoint.reset();
            redraw();
        }
        break;
        default:
            break;
        }
    }
    return true;
}

bool NotebookPage::on_motion_notify_event(GdkEventMotion *motion)
{
    lastMouse.first = motion->x;
    lastMouse.second = motion->y;
    return true;
}

bool NotebookPage::on_scroll_event(GdkEventScroll *scroll)
{
    viewRect.expand(viewRect.w * scroll->delta_y * 0.01, viewRect.h * scroll->delta_y * 0.01);
    redraw();
    return true;
}

bool NotebookPage::on_leave_notify_event(GdkEventCrossing *)
{
    movePoint.reset();
    redraw();
    return true;
}

void NotebookPage::redraw()
{
    if (!timer || !timer->is_connected())
    {
        timer.emplace(*this);
    }
}

} // namespace CanForm