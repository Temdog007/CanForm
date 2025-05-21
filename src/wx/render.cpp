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
    }
    else
    {
        wxBrush *brush = wxTheBrushList->FindOrCreateBrush(getColor(style.color));
        gc.SetBrush(*brush);
    }
}

void NotebookPage::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);

    wxGraphicsContext *gc = dc.GetGraphicsContext();
    if (gc)
    {
        for (const auto &atom : atoms)
        {
            setColor(*gc, atom.style);
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