#include <canform.hpp>
#include <filesystem>
#include <sstream>
#include <wx/wx.h>

using namespace CanForm;

class MainFrame : public wxFrame, public FileDialog::Handler
{
  private:
    void OnOpenFile(wxCommandEvent &);
    void OnOpenDir(wxCommandEvent &);
    void OnSave(wxCommandEvent &);
    void OnExit(wxCommandEvent &);

    void OnTest(wxCommandEvent &);

  public:
    MainFrame();
    virtual ~MainFrame()
    {
    }

    virtual bool handle(std::string_view) override;

    DECLARE_EVENT_TABLE()
};

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "CanForm wxWidgets Test")
{
    wxMenuBar *bar = new wxMenuBar();

    wxMenu *file = new wxMenu();

    file->Append(wxID_OPEN);
    file->Append(wxID_FILE, wxT("Open Directory"));
    file->AppendSeparator();
    file->Append(wxID_SAVE);
    file->AppendSeparator();
    file->Append(wxID_EXIT);

    bar->Append(file, wxT("&File"));

    wxMenu *test = new wxMenu();

    test->Append(wxID_FILE2, wxT("Show Test Form"));

    bar->Append(test, wxT("Test"));

    SetMenuBar(bar);
}

bool MainFrame::handle(std::string_view file)
{
    showMessageBox(MessageBoxType::Information, "Got File/Directory", file, this);
    return true;
}

void MainFrame::OnOpenFile(wxCommandEvent &)
{
    FileDialog dialog;
    dialog.message = "Select directory(s)";
    dialog.startDirectory = std::filesystem::current_path().c_str();
    dialog.multiple = true;
    dialog.show(*this, this);
}

void MainFrame::OnOpenDir(wxCommandEvent &)
{
    FileDialog dialog;
    dialog.message = "Select file(s)";
    dialog.startDirectory = std::filesystem::current_path().c_str();
    dialog.multiple = true;
    dialog.directories = true;
    dialog.show(*this, this);
}

void MainFrame::OnSave(wxCommandEvent &)
{
    FileDialog dialog;
    dialog.message = "Select file(s)";
    dialog.startDirectory = std::filesystem::current_path().c_str();
    dialog.saving = true;
    dialog.show(*this, this);
}

void MainFrame::OnExit(wxCommandEvent &)
{
    Close(true);
}

struct Printer
{
    std::ostream &os;

    constexpr Printer(std::ostream &os) noexcept : os(os)
    {
    }

    std::ostream &operator()(const StringSet &)
    {
        return os;
    }

    std::ostream &operator()(const StringMap &)
    {
        return os;
    }

    template <typename T> std::ostream &operator()(const T &t)
    {
        return os << t;
    }
};

void MainFrame::OnTest(wxCommandEvent &)
{
    std::pmr::memory_resource *resource = std::pmr::new_delete_resource();
    Form form;
    form["Flag"] = false;
    form["Number"] = 0;
    form["String"] = String("Hello", resource);
    switch (executeForm("Test Form", form, this))
    {
    case DialogResult::Ok: {
        std::ostringstream os;
        for (const auto &[name, data] : form)
        {
            os << name << ": ";
            std::visit(Printer(os), data);
            os << std::endl;
        }
        showMessageBox(MessageBoxType::Information, "Ok", os.str(), this);
    }
    break;
    case DialogResult::Cancel:
        showMessageBox(MessageBoxType::Warning, "Cancel", "Form Canceled", this);
        break;
    default:
        showMessageBox(MessageBoxType::Error, "Error", "Form Failed", this);
        break;
    }
}

class MyApp : public wxApp
{
    virtual ~MyApp()
    {
    }

    virtual bool OnInit() override
    {
        MainFrame *frame = new MainFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame) EVT_MENU(wxID_OPEN, MainFrame::OnOpenFile)
    EVT_MENU(wxID_FILE, MainFrame::OnOpenDir) EVT_MENU(wxID_SAVE, MainFrame::OnSave)
        EVT_MENU(wxID_EXIT, MainFrame::OnExit) EVT_MENU(wxID_FILE2, MainFrame::OnTest) wxEND_EVENT_TABLE()