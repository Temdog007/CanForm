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

    void OnModalTest(wxCommandEvent &);
    void OnNonModalTest(wxCommandEvent &);

    static Form makeForm();
    static void printForm(const Form &, DialogResult, wxWindow *parent = nullptr);

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

    test->Append(wxID_FILE2, wxT("Show Modal Form"));
    test->Append(wxID_FILE3, wxT("Show Non-Modal Form"));

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

    std::ostream &operator()(const StringSelection &s)
    {
        os << "Selected " << s.index << std::endl;
        for (const auto &name : s.set)
        {
            os << '\t' << name << std::endl;
        }
        return os;
    }

    std::ostream &operator()(const StringMap &map)
    {
        for (const auto &[name, flag] : map)
        {
            os << name << ": " << flag << std::endl;
        }
        return os;
    }

    template <typename T> std::ostream &operator()(const T &t)
    {
        return os << t;
    }
};

Form MainFrame::makeForm()
{
    std::pmr::memory_resource *resource = std::pmr::new_delete_resource();
    Form form;
    form["Flag"] = false;
    form["Number"] = 0;
    form["String"] = String("Hello", resource);

    StringSelection selection;
    constexpr std::array<std::string_view, 5> Classes = {"Mammal", "Bird", "Reptile", "Amphibian", "Fish"};
    for (auto cls : Classes)
    {
        selection.set.emplace(cls);
    }
    form["Single Selection"] = std::move(selection);

    constexpr std::array<std::string_view, 4> Actions = {"Climb", "Swim", "Fly", "Dig"};
    StringMap map;
    for (auto a : Actions)
    {
        map.emplace(a, rand() % 2 == 0);
    }
    form["Multiple Selections"] = std::move(map);
    return form;
}

void MainFrame::printForm(const Form &form, DialogResult result, wxWindow *parent)
{
    switch (result)
    {
    case DialogResult::Ok: {
        std::ostringstream os;
        for (const auto &[name, data] : form)
        {
            os << name << " → ";
            std::visit(Printer(os), data);
            os << std::endl;
        }
        showMessageBox(MessageBoxType::Information, "Ok", os.str(), parent);
    }
    break;
    case DialogResult::Cancel:
        showMessageBox(MessageBoxType::Warning, "Cancel", "Form Canceled", parent);
        break;
    default:
        showMessageBox(MessageBoxType::Error, "Error", "Form Failed", parent);
        break;
    }
}

void MainFrame::OnNonModalTest(wxCommandEvent &)
{
    struct Test : public AsyncForm
    {
        virtual ~Test()
        {
        }

        virtual void onSubmit(DialogResult result) override
        {
            MainFrame::printForm(form, result);
        }
    };
    std::shared_ptr<AsyncForm> asyncForm = std::make_shared<Test>();
    asyncForm->form = makeForm();
    AsyncForm::show(asyncForm, "Non Modal Form", this);
}

void MainFrame::OnModalTest(wxCommandEvent &)
{
    Form form = makeForm();
    printForm(form, executeForm("Modal Form", form, this), this);
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

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame) EVT_MENU(wxID_OPEN, MainFrame::OnOpenFile)
    EVT_MENU(wxID_FILE, MainFrame::OnOpenDir) EVT_MENU(wxID_SAVE, MainFrame::OnSave)
        EVT_MENU(wxID_EXIT, MainFrame::OnExit) EVT_MENU(wxID_FILE2, MainFrame::OnModalTest)
            EVT_MENU(wxID_FILE3, MainFrame::OnNonModalTest) wxEND_EVENT_TABLE()