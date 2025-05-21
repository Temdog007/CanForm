#include <wx/colour.h>
#include <wx/dcbuffer.h>
#include <wx/wx.hpp>

namespace CanForm
{
NotebookPage::NotebookPage(wxWindow *parent) : wxPanel(parent), atoms(), matrix()
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
    bool fill;

    constexpr Drawer(wxGraphicsContext &gc) noexcept : gc(gc), fill(false)
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
        gc.DrawText(convert(t.string), t.x, t.y);
    }

    void operator()(const RenderAtom &atom)
    {
        setColor(gc, atom.style);
        std::visit(*this, atom.renderType);
    }
};

void NotebookPage::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);

    wxGraphicsContext *gc = dc.GetGraphicsContext();
    if (gc)
    {
        Drawer drawer(*gc);
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

wxBEGIN_EVENT_TABLE(NotebookPage, wxWindow) EVT_PAINT(NotebookPage::OnPaint) wxEND_EVENT_TABLE();
} // namespace CanForm