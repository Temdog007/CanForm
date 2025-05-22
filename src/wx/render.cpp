#include <wx/colour.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/wx.hpp>

namespace CanForm
{
std::pmr::unordered_set<NotebookPage *> NotebookPage::pages;

NotebookPage::NotebookPage(wxWindow *parent)
    : wxPanel(parent), atoms(), viewRect(), lastMouse(), captureState(std::nullopt)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
}

NotebookPage::~NotebookPage()
{
    pages.erase(this);
}

NotebookPage *NotebookPage::create(wxWindow *parent)
{
    NotebookPage *page = new NotebookPage(parent);
    pages.insert(page);
    return page;
}

NotebookPage::CaptureState::CaptureState(wxWindow &w) : window(w)
{
    window.CaptureMouse();
    window.SetCursor(*wxCROSS_CURSOR);
}

NotebookPage::CaptureState::~CaptureState()
{
    if (window.HasCapture())
    {
        window.ReleaseMouse();
        window.SetCursor(wxNullCursor);
    }
}

void NotebookPage::OnCaptureLost(wxMouseCaptureLostEvent &)
{
    captureState.reset();
    SetCursor(wxNullCursor);
}

wxColour getColor(Color c) noexcept
{
    return wxColour(c.red, c.green, c.blue, c.alpha);
}

void setColor(wxGraphicsContext &gc, const RenderStyle &style)
{
    if (style.fill)
    {
        wxBrush *brush = wxTheBrushList->FindOrCreateBrush(getColor(style.color));
        gc.SetBrush(*brush);
        gc.SetPen(*wxTRANSPARENT_PEN);
    }
    else
    {
        wxPen *pen = wxThePenList->FindOrCreatePen(getColor(style.color));
        gc.SetPen(*pen);
        gc.SetBrush(*wxTRANSPARENT_BRUSH);
    }
}

struct Drawer
{
    wxGraphicsContext &gc;
    wxFont font;
    const RenderStyle *style;

    Drawer(wxGraphicsContext &g, wxFont &&f) noexcept : gc(g), font(std::move(f)), style(nullptr)
    {
    }

    void operator()(const CanFormRectangle &r)
    {
        gc.DrawRectangle(r.x, r.y, r.w, r.h);
    }
    void operator()(const RoundedRectangle &rr)
    {
        auto &r = rr.rectangle;
        gc.DrawRoundedRectangle(r.x, r.y, r.w, r.h, rr.radius);
    }
    void operator()(const Ellipse &r)
    {
        gc.DrawEllipse(r.x, r.y, r.w, r.h);
    }
    void operator()(const Text &t)
    {
        gc.SetFont(font, getColor(style->color));
        gc.DrawText(convert(t.string), t.x, t.y);
    }

    void operator()(const RenderAtom &atom)
    {
        setColor(gc, atom.style);
        style = &atom.style;
        std::visit(*this, atom.renderType);
    }
};

void NotebookPage::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);

    wxGraphicsContext *gc = dc.GetGraphicsContext();
    if (gc)
    {
        const wxSize size = GetSize();

        wxGraphicsMatrix matrix = gc->CreateMatrix();
        const auto [cx, cy] = viewRect.center();

        matrix.Translate(size.GetWidth() * 0.5, size.GetHeight() * 0.5);
        matrix.Scale(viewRect.w / size.GetWidth(), viewRect.h / size.GetHeight());
        matrix.Translate(size.GetWidth() * -0.5, size.GetHeight() * -0.5);

        matrix.Translate(cx - size.GetWidth() * 0.5, cy - size.GetHeight() * 0.5);

        gc->SetTransform(matrix);

        Drawer drawer(*gc, GetFont());
        for (const auto &atom : atoms)
        {
            drawer(atom);
        }
        gc->Flush();
    }
}

bool getCanvasAtoms(std::string_view canvas, RenderAtomsUser &users, void *ptr)
{
    wxWindow *parent = (wxWindow *)ptr;

    bool found = false;
    const wxString target = convert(canvas);
    NotebookPage::forEachPage([&](NotebookPage &page) {
        wxWindow *parent = page.GetParent();
        if (auto book = dynamic_cast<wxNotebook *>(parent))
        {
            const size_t count = book->GetPageCount();
            for (size_t i = 0; i < count; ++i)
            {
                if (target == book->GetPageText(i))
                {
                    users.use(page.atoms, page.viewRect);
                    found = true;
                    return;
                }
            }
        }
        if (auto window = dynamic_cast<wxTopLevelWindow *>(parent))
        {
            if (window->GetTitle() == target)
            {
                users.use(page.atoms, page.viewRect);
                found = true;
                return;
            }
        }
    });
    if (found)
    {
        return true;
    }
    if (auto book = dynamic_cast<wxNotebook *>(parent))
    {
        NotebookPage *page = NotebookPage::create(book);
        if (book->AddPage(page, target, true))
        {
            users.use(page->atoms, page->viewRect);
            return true;
        }
    }

    wxFrame *frame = new wxFrame(nullptr, wxID_ANY, target);
    NotebookPage *page = NotebookPage::create(frame);
    users.use(page->atoms, page->viewRect);
    frame->Show();

    return true;
}

void NotebookPage::reset()
{
    viewRect.x = 0;
    viewRect.y = 0;
    const wxSize size = GetSize();
    viewRect.w = size.GetWidth();
    viewRect.h = size.GetHeight();
    Refresh();
}

std::pair<wxNotebook *, size_t> NotebookPage::getBook() const
{
    wxNotebook *book = dynamic_cast<wxNotebook *>(GetParent());
    if (book)
    {
        const size_t n = book->GetPageCount();
        for (size_t i = 0; i < n; ++i)
        {
            if (book->GetPage(i) == this)
            {
                return std::make_pair(book, i);
            }
        }
    }
    return std::make_pair(nullptr, std::numeric_limits<size_t>::max());
}

void NotebookPage::close()
{
    auto [book, page] = getBook();
    if (book == nullptr)
    {
        Close(true);
    }
    else
    {
        book->DeletePage(page);
    }
}

void NotebookPage::moveToWindow()
{
    auto [book, page] = getBook();
    if (book == nullptr)
    {
        wxTopLevelWindow *parent = static_cast<wxTopLevelWindow *>(GetParent());
        wxWindow *window = wxTheApp->GetTopWindow();
        for (auto child : window->GetChildren())
        {
            if (auto book = dynamic_cast<wxNotebook *>(child))
            {
                Reparent(book);
                book->AddPage(this, parent->GetTitle(), true);
                break;
            }
        }
        parent->Close(true);
    }
    else
    {
        wxFrame *frame = new wxFrame(nullptr, wxID_ANY, book->GetPageText(page));
        book->RemovePage(page);
        Reparent(frame);
        frame->Show();
    }
}

void NotebookPage::OnMenu(wxCommandEvent &e)
{
    switch (e.GetId())
    {
    case wxID_REVERT:
        reset();
        break;
    case wxID_CLOSE:
        close();
        break;
    case wxID_RESTORE_FRAME:
        moveToWindow();
        break;
    default:
        break;
    }
}

void NotebookPage::OnMouse(wxMouseEvent &e)
{
    const wxPoint point = wxGetMousePosition();
    const wxPoint screen = GetScreenPosition();
    const wxPoint current = point - screen;
    if (e.IsButton())
    {
        if (e.Button(wxMOUSE_BTN_MIDDLE) && e.ButtonDown(wxMOUSE_BTN_MIDDLE))
        {
            if (HasCapture())
            {
                captureState.reset();
            }
            else
            {
                captureState.emplace(*this);
            }
        }
        else if (e.Button(wxMOUSE_BTN_RIGHT) && e.ButtonDown(wxMOUSE_BTN_RIGHT))
        {
            wxMenu menu;
            menu.Append(wxID_REVERT, "Reset View");
            menu.AppendSeparator();
            auto [book, _] = getBook();
            if (book == nullptr)
            {
                menu.Append(wxID_RESTORE_FRAME, "Pop In");
            }
            else
            {
                menu.Append(wxID_RESTORE_FRAME, "Pop Out");
            }
            menu.AppendSeparator();
            menu.Append(wxID_CLOSE);
            menu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(NotebookPage::OnMenu), nullptr, this);
            PopupMenu(&menu);
        }
    }
    else if (HasCapture())
    {
        const wxPoint delta = current - lastMouse;
        viewRect.x += delta.x;
        viewRect.y += delta.y;
        Refresh();
    }
    else if (e.Leaving() && HasCapture())
    {
        captureState.reset();
    }
    int wheel = e.GetWheelRotation();
    if (wheel != 0)
    {
        viewRect.expand(viewRect.w * wheel * 0.0001, viewRect.h * wheel * 0.0001);
        viewRect.w = std::clamp(viewRect.w, 1.0, 10000.0);
        viewRect.h = std::clamp(viewRect.h, 1.0, 10000.0);
        Refresh();
    }
    lastMouse = current;
}

wxBEGIN_EVENT_TABLE(NotebookPage, wxWindow) EVT_PAINT(NotebookPage::OnPaint) EVT_MOUSE_EVENTS(NotebookPage::OnMouse)
    EVT_MOUSE_CAPTURE_LOST(NotebookPage::OnCaptureLost) wxEND_EVENT_TABLE();
} // namespace CanForm