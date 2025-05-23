#include <canform.hpp>
#include <filesystem>
#include <sstream>
#include <tests/test.hpp>
#include <wx/dc.h>
#include <wx/wx.hpp>

using namespace CanForm;

class MainFrame : public wxFrame, public FileDialog::Handler, public RenderAtomsUser
{
  private:
    void OnTool(wxCommandEvent &);

    wxNotebook *book;

  public:
    MainFrame();
    virtual ~MainFrame();

    virtual bool handle(std::string_view) override;
    virtual void use(RenderAtoms &, CanFormRectangle &) override;

    DECLARE_EVENT_TABLE()
};

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "CanForm wxWidgets Test"), book(new wxNotebook(this, wxID_ANY))
{
    wxToolBar *bar = CreateToolBar();
    auto bundle = wxBitmapBundle::FromSVG("<svg version='1.1' xmlns='http://www.w3.org/2000/svg'>"
                                          "<rect width='100%' height='100% fill='red' />"
                                          "</svg>",
                                          wxDefaultSize);
    bar->AddTool(wxID_ANY, wxT("Menu"), bundle, wxT("Show Menu"));
    bar->Realize();

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(book, 1, wxEXPAND, 10);

    SetSizerAndFit(sizer);

    SetDoubleBuffered(true);
}

MainFrame::~MainFrame()
{
}

bool MainFrame::handle(std::string_view file)
{
    showMessageBox(MessageBoxType::Information, "Got File/Directory", file, this);
    return true;
}

void MainFrame::OnTool(wxCommandEvent &)
{
    MenuList menuList;
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "File";
        menu.add("Open File", [this]() {
            FileDialog dialog;
            dialog.message = "Select directory(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.show(*this, this);
            return true;
        });
        menu.add("Open Directory", [this]() {
            FileDialog dialog;
            dialog.message = "Select file(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.directories = true;
            dialog.show(*this, this);
            return true;
        });
        menu.add("Save File", [this]() {
            FileDialog dialog;
            dialog.message = "Select file(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.saving = true;
            dialog.show(*this, this);
            return true;
        });
        menu.add("Exit", [this]() {
            Close(true);
            return true;
        });
    }
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Tests";
        menu.add("Modal Form", [this]() {
            Form form = makeForm();
            printForm(form, executeForm("Modal Form", form, this), this);
            return false;
        });
        menu.add("Non Modal Form", [this]() {
            showAsyncForm(
                makeForm(), "Non Modal Form", [](Form &form, DialogResult result) { printForm(form, result); }, this);
            return false;
        });
        menu.add("Add Canvas", [this]() {
            const wxString string = randomString(5, 10);
            getCanvasAtoms(toView(string), *this, book);
            return true;
        });
    }

    menuList.show("Main Menu", this);
}

struct RandomRender
{
    wxSize size;

    RandomRender(const wxSize &s) noexcept : size(s)
    {
    }

    void randomPosition(double &x, double &y)
    {
        x = rand() % size.GetWidth();
        y = rand() % size.GetHeight();
    }

    void operator()(RenderStyle &style)
    {
        style.color.red = rand() % 256;
        style.color.green = rand() % 256;
        style.color.blue = rand() % 256;
        style.color.alpha = 255u;
        style.fill = rand() % 2 == 0;
    }

    void operator()(CanFormRectangle &r)
    {
        randomPosition(r.x, r.y);
        r.w = rand() % 50 + 10;
        r.h = rand() % 50 + 10;
    }

    void operator()(CanFormEllipse &r)
    {
        randomPosition(r.x, r.y);
        r.w = rand() % 50 + 10;
        r.h = rand() % 50 + 10;
    }

    void operator()(RoundedRectangle &r)
    {
        operator()(r.rectangle);
        r.radius = std::min(r.rectangle.w, r.rectangle.h) * ((double)rand() / RAND_MAX) * 0.4 + 0.1;
    }

    void operator()(Text &t)
    {
        randomPosition(t.x, t.y);
        t.string = randomString(5, 10).ToStdString();
    }

    RenderAtom operator()()
    {
        RenderAtom atom;
        operator()(atom.style);
        switch (rand() % 4)
        {
        case 0: {
            auto &e = atom.renderType.emplace<CanFormEllipse>();
            operator()(e);
        }
        break;
        case 1: {
            auto &r = atom.renderType.emplace<RoundedRectangle>();
            operator()(r);
        }
        break;
        case 3: {
            auto &t = atom.renderType.emplace<Text>();
            operator()(t);
        }
        break;
        default: {
            auto &r = atom.renderType.emplace<CanFormRectangle>();
            operator()(r);
        }
        break;
        }
        return atom;
    }
};

void MainFrame::use(RenderAtoms &atoms, CanFormRectangle &viewRect)
{
    atoms.clear();

    viewRect.x = 0;
    viewRect.y = 0;
    const auto size = GetSize();
    viewRect.w = size.GetWidth();
    viewRect.h = size.GetHeight();

    {
        RenderAtom atom;
        atom.renderType.emplace<CanFormRectangle>(viewRect);
        atoms.emplace_back(std::move(atom));
    }

    const size_t n = rand() % 50 + 50;
    RandomRender random(GetSize());
    for (size_t i = 0; i < n; ++i)
    {
        atoms.emplace_back(random());
    }

    Refresh();
}

class MyApp : public wxApp
{
    virtual ~MyApp()
    {
    }

    virtual bool OnInit() override
    {
        if (!wxApp::OnInit())
        {
            return false;
        }
        srand(time(nullptr));
        MainFrame *frame = new MainFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame) EVT_TOOL(wxID_ANY, MainFrame::OnTool) wxEND_EVENT_TABLE();