#include <wx/colour.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/wx.hpp>

namespace CanForm
{
NotebookPage::NotebookPage(wxWindow *parent) : wxPanel(parent), atoms(), matrix(), lastMouse()
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

wxColour getColor(Color c) noexcept
{
    return wxColour(c.red, c.green, c.blue, c.alpha);
}

void setColor(wxGraphicsContext &gc, const RenderStyle &style)
{
    if (style.fill)
    {
        wxPen *pen = wxThePenList->FindOrCreatePen(getColor(style.color));
        gc.SetPen(*pen);
        gc.SetBrush(*wxTRANSPARENT_BRUSH);
    }
    else
    {
        wxBrush *brush = wxTheBrushList->FindOrCreateBrush(getColor(style.color));
        gc.SetBrush(*brush);
        gc.SetPen(*wxTRANSPARENT_PEN);
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

    void operator()(const Rectangle &r)
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
        gc->SetTransform(gc->CreateMatrix(matrix));
        Drawer drawer(*gc, GetFont());
        for (const auto &atom : atoms)
        {
            drawer(atom);
        }
        gc->Flush();
    }
}

bool getCanvasAtoms(std::string_view canvas, RenderAtomsUser &users, bool createIfNeeded)
{
    const wxString target = convert(canvas);
    const size_t count = gNotebook->GetPageCount();

    for (size_t i = 0; i < count; ++i)
    {
        if (target == gNotebook->GetPageText(i))
        {
            NotebookPage *page = dynamic_cast<NotebookPage *>(gNotebook->GetPage(i));
            if (page != nullptr)
            {
                users.use(page->atoms);
                return true;
            }
        }
    }

    if (createIfNeeded)
    {
        NotebookPage *page = new NotebookPage(gNotebook);
        if (gNotebook->AddPage(page, target, true))
        {
            users.use(page->atoms);
            return true;
        }
    }
    return false;
}

void NotebookPage::OnMouse(wxMouseEvent &e)
{
    const wxPoint point = wxGetMousePosition();
    const wxPoint screen = GetScreenPosition();
    const wxPoint current = point - screen;
    if (e.IsButton() && e.Button(wxMOUSE_BTN_MIDDLE) && e.ButtonDown(wxMOUSE_BTN_MIDDLE))
    {
        if (HasCapture())
        {
            ReleaseMouse();
        }
        else
        {
            CaptureMouse();
        }
    }
    else if (HasCapture())
    {
        const wxPoint delta = current - lastMouse;
        matrix.Translate(delta.x, delta.y);
        Refresh();
    }
    else if (e.Leaving() && HasCapture())
    {
        ReleaseMouse();
    }
    lastMouse = current;
}

wxBEGIN_EVENT_TABLE(NotebookPage, wxWindow) EVT_PAINT(NotebookPage::OnPaint) EVT_MOUSE_EVENTS(NotebookPage::OnMouse)
    wxEND_EVENT_TABLE();
} // namespace CanForm