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
    virtual void use(void *, RenderAtoms &, CanFormRectangle &) override;

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
            getCanvasAtoms(randomString(5, 10), *this, book);
            return true;
        });
    }

    menuList.show("Main Menu", this);
}

void MainFrame::use(void *ptr, RenderAtoms &atoms, CanFormRectangle &viewRect)
{
    NotebookPage *page = (NotebookPage *)ptr;

    atoms.clear();

    viewRect.x = 0;
    viewRect.y = 0;
    const auto size = page->GetSize();
    viewRect.w = size.GetWidth();
    viewRect.h = size.GetHeight();

    {
        RenderAtom atom;
        atom.renderType.emplace<CanFormRectangle>(viewRect);
        atoms.emplace_back(std::move(atom));
    }

    const size_t n = rand() % 50 + 50;
    RandomRender random(viewRect.w, viewRect.h);
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