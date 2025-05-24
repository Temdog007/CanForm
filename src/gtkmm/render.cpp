#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>

namespace CanForm
{
std::pmr::unordered_set<NotebookPage *> NotebookPage::pages;

NotebookPage::NotebookPage()
    : Gtk::DrawingArea(), atoms(), viewRect(), movePoint(std::nullopt), lastMouse(0, 0), timer(std::nullopt),
      clearColor()
{
    clearColor.red = 0.5;
    clearColor.green = 0.5;
    clearColor.blue = 0.5;
    clearColor.alpha = 1.0;
    add_events(Gdk::POINTER_MOTION_MASK);
    add_events(Gdk::BUTTON_PRESS_MASK);
    add_events(Gdk::BUTTON_RELEASE_MASK);
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

constexpr int UpdateRate = 10;
constexpr double UpdateRateInMilliseconds = UpdateRate * 0.001;

bool NotebookPage::Update(int)
{
    bool again = false;
    if (movePoint)
    {
        const auto &point = *movePoint;
        viewRect.x += clampIfSmall((point.first - lastMouse.first)) * UpdateRateInMilliseconds;
        viewRect.y += clampIfSmall((point.second - lastMouse.second)) * UpdateRateInMilliseconds;
        again = true;
    }
    queue_draw();
    return again;
}

NotebookPage::Timer::Timer(NotebookPage &page) : connection()
{
    sigc::slot<bool> slot = sigc::bind(sigc::mem_fun(page, &NotebookPage::Update), 0);
    connection = Glib::signal_timeout().connect(slot, UpdateRate);
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

    Drawer(const Cairo::RefPtr<Cairo::Context> &c) : ctx(c)
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
        auto layout = Pango::Layout::create(ctx);
        layout->set_markup(convert(t.string));
        ctx->move_to(t.x, t.y);
        layout->show_in_cairo_context(ctx);
        return false;
    }
    void operator()(const RenderAtom &atom)
    {
        const RenderStyle &style = atom.style;
        ctx->save();
        ctx->set_source_rgba(style.color.red, style.color.green, style.color.blue, style.color.alpha);
        if (std::visit(*this, atom.renderType))
        {
            if (style.lineWidth)
            {
                ctx->set_line_width(*style.lineWidth);
                ctx->stroke();
            }
            else
            {
                ctx->fill();
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
    if (viewRect.w < viewBounds.w)
    {
        viewRect.x = std::clamp(viewRect.x, viewBounds.x, (viewBounds.x + viewBounds.w) - viewRect.w);
    }
    else
    {
        viewRect.x = viewBounds.x + (viewBounds.w - viewRect.w) * 0.5;
    }
    if (viewRect.h < viewBounds.h)
    {
        viewRect.y = std::clamp(viewRect.y, viewBounds.y, (viewBounds.y + viewBounds.h) - viewRect.h);
    }
    else
    {
        viewRect.y = viewBounds.y + (viewBounds.h - viewRect.h) * 0.5;
    }

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

        const double rx = lastMouse.first - movePoint->first;
        const double ry = lastMouse.second - movePoint->second;

        const double angle = angleBetween(1, 0, rx, ry);
        ctx->rotate(angle);

        ctx->set_source_rgb(1.0, 1.0, 1.0);
        ctx->begin_new_path();
        ctx->arc(0, 0, 20.0, 0, M_PI * 2.0);
        ctx->fill_preserve();
        ctx->set_source_rgb(0.0, 0.0, 0.0);
        ctx->stroke();

        ctx->move_to(-10, -10);
        ctx->line_to(-10, 10);
        ctx->line_to(15, 0);
        ctx->fill();

        ctx->move_to(0, 0);
        ctx->line_to(std::sqrt(rx * rx + ry * ry), 0);
        ctx->stroke();
    }
    return true;
}

std::pair<double, double> getTextSize(std::string_view s) noexcept
{
    std::pair<double, double> pair(0, 0);
    if (cairoContext)
    {
        auto layout = Pango::Layout::create(cairoContext);
        layout->set_markup(convert(s));
        int width;
        int height;
        layout->get_pixel_size(width, height);
        pair.first = width;
        pair.second = height;
    }
    return pair;
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
                    users.use(&page, page.atoms, page.viewRect, page.viewBounds);
                    book->set_current_page(i);
                    found = true;
                    return;
                }
            }
        }
        if (auto window = dynamic_cast<Gtk::Window *>(parent))
        {
            if (window->get_title() == target)
            {
                users.use(&page, page.atoms, page.viewRect, page.viewBounds);
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
        page->appendToNotebook(*book, canvas);
        users.use(page, page->atoms, page->viewRect, page->viewBounds);
        book->show_all_children();
        book->set_current_page(book->get_n_pages() - 1);
        return true;
    }

    return false;
}

void NotebookPage::appendToNotebook(Gtk::Notebook &book, std::string_view tabName)
{
    Gtk::HBox *box = Gtk::manage(new Gtk::HBox());

    Gtk::Label *label = Gtk::manage(new Gtk::Label(convert(tabName)));
    box->pack_start(*label, Gtk::PACK_EXPAND_WIDGET);

    Gtk::Button *button = Gtk::manage(new Gtk::Button("✖"));
    button->set_relief(Gtk::RELIEF_NONE);
    button->signal_clicked().connect([this, &book]() { book.remove_page(*this); });
    box->pack_start(*button, Gtk::PACK_SHRINK);

    box->set_spacing(10);

    box->show_all_children();

    label = Gtk::manage(new Gtk::Label(convert(tabName)));
    book.append_page(*this, *box, *label);
}

bool NotebookPage::on_button_press_event(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS)
    {
        switch (event->button)
        {
        case 1:
            movePoint.emplace(event->x, event->y);
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

bool NotebookPage::on_button_release_event(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_RELEASE)
    {
        switch (event->button)
        {
        case 1:
            movePoint.reset();
            redraw();
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