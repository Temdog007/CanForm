#include <canform.hpp>
#include <filesystem>
#include <sstream>
#include <wx/dc.h>
#include <wx/wx.hpp>

using namespace CanForm;

wxNotebook *CanForm::gNotebook = nullptr;

class MainFrame : public wxFrame, public FileDialog::Handler, public RenderAtomsUser
{
  private:
    void OnOpenFile(wxCommandEvent &);
    void OnOpenDir(wxCommandEvent &);
    void OnSave(wxCommandEvent &);
    void OnExit(wxCommandEvent &);

    void OnModalTest(wxCommandEvent &);
    void OnNonModalTest(wxCommandEvent &);
    void OnAddCanvas(wxCommandEvent &);

    static Form makeForm();
    static void printForm(const Form &, DialogResult, wxWindow *parent = nullptr);

  public:
    MainFrame();
    virtual ~MainFrame();

    virtual bool handle(std::string_view) override;
    virtual void use(RenderAtoms &, CanFormRectangle &) override;

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
    test->Append(wxID_FILE4, wxT("Add Canvas"));

    bar->Append(test, wxT("Test"));

    SetMenuBar(bar);

    gNotebook = new wxNotebook(this, wxID_OK);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(gNotebook, 1, wxEXPAND, 10);

    SetSizerAndFit(sizer);

    SetDoubleBuffered(true);
}

MainFrame::~MainFrame()
{
    gNotebook = nullptr;
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
    auto s = std::filesystem::current_path().string();
    dialog.startDirectory = s;
    dialog.multiple = true;
    dialog.show(*this, this);
}

void MainFrame::OnOpenDir(wxCommandEvent &)
{
    FileDialog dialog;
    dialog.message = "Select file(s)";
    auto s = std::filesystem::current_path().string();
    dialog.startDirectory = s;
    dialog.multiple = true;
    dialog.directories = true;
    dialog.show(*this, this);
}

void MainFrame::OnSave(wxCommandEvent &)
{
    FileDialog dialog;
    dialog.message = "Select file(s)";
    auto s = std::filesystem::current_path().string();
    dialog.startDirectory = s;
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
        int i = 0;
        for (const auto &name : s.set)
        {
            os << '\t' << name;
            if (i == s.index)
            {
                os << "✔";
            }
            os << std::endl;
            ++i;
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

    std::ostream &operator()(const Number &n)
    {
        return std::visit(*this, n);
    }

    std::ostream &operator()(const MultiForm &multi)
    {
        os << "Selected " << multi.selected << std::endl;
        auto iter = multi.tabs.find(multi.selected);
        if (iter != multi.tabs.end())
        {
            const auto &form = iter->second;
            for (const auto &[name, formData] : *form)
            {
                os << "Entry: " << name << " → ";
                std::visit(*this, *formData);
                os << std::endl;
            }
        }
        return os;
    }

    std::ostream &operator()(int8_t t)
    {
        return operator()(static_cast<ssize_t>(t));
    }

    std::ostream &operator()(uint8_t t)
    {
        return operator()(static_cast<size_t>(t));
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
    form["Signed Integer"] = 0;
    form["Unsigned Integer"] = 0u;
    form["Float"] = 0.f;
    form["String"] = String("Hello", resource);

    constexpr std::array<std::string_view, 5> Classes = {"Mammal", "Bird", "Reptile", "Amphibian", "Fish"};
    form["Single Selection"] = StringSelection(Classes);

    constexpr std::array<std::string_view, 4> Actions = {"Climb", "Swim", "Fly", "Dig"};
    StringMap map;
    for (auto a : Actions)
    {
        map.emplace(a, rand() % 2 == 0);
    }
    form["Multiple Selections"] = std::move(map);

    MultiForm multi;
    multi.tabs["Extra1"] =
        Form::create("Age", static_cast<uint8_t>(42), "Favorite Color", StringSelection({"Red", "Green", "Blue"}));
    multi.tabs["Extra2"] = Form::create("Active", true, "Weight", 0.0, "Sports",
                                        createStringMap("Basketball", "Football", "Golf", "Polo"));
    multi.selected = "Extra1";
    form["Extra"] = std::move(multi);
    return form;
}

void MainFrame::printForm(const Form &form, DialogResult result, wxWindow *parent)
{
    switch (result)
    {
    case DialogResult::Ok: {
        std::ostringstream os;
        for (const auto &[name, data] : *form)
        {
            os << name << " → ";
            std::visit(Printer(os), *data);
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
    showAsyncForm(
        makeForm(), "Non Modal Form", [](Form &form, DialogResult result) { printForm(form, result); }, this);
}

void MainFrame::OnModalTest(wxCommandEvent &)
{
    Form form = makeForm();
    printForm(form, executeForm("Modal Form", form, this), this);
}

void MainFrame::OnAddCanvas(wxCommandEvent &)
{
    const wxString string = randomString(5, 10);
    getCanvasAtoms(toView(string), *this, true);
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

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame) EVT_MENU(wxID_OPEN, MainFrame::OnOpenFile)
    EVT_MENU(wxID_FILE, MainFrame::OnOpenDir) EVT_MENU(wxID_SAVE, MainFrame::OnSave)
        EVT_MENU(wxID_EXIT, MainFrame::OnExit) EVT_MENU(wxID_FILE2, MainFrame::OnModalTest)
            EVT_MENU(wxID_FILE3, MainFrame::OnNonModalTest) EVT_MENU(wxID_FILE4, MainFrame::OnAddCanvas)
                wxEND_EVENT_TABLE();