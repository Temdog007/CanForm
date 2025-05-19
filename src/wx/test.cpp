#include <canform.hpp>
#include <filesystem>
#include <wx/wx.h>

using namespace CanForm;

class MainFrame : public wxFrame, public FileDialog::Handler
{
  private:
    void OnOpenFile(wxCommandEvent &);
    void OnOpenDir(wxCommandEvent &);
    void OnSave(wxCommandEvent &);
    void OnExit(wxCommandEvent &);

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
        EVT_MENU(wxID_EXIT, MainFrame::OnExit) wxEND_EVENT_TABLE()