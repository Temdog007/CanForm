#include <wx/notebook.h>
#include <wx/sstream.h>
#include <wx/valnum.h>
#include <wx/window.h>
#include <wx/windowptr.h>

#include <wx/wx.hpp>

#include <fstream>

namespace CanForm
{
wxString convert(std::string_view string)
{
    return wxString::FromUTF8(string.data(), string.size());
}

std::string_view toView(const wxString &string) noexcept
{
    return std::string_view(string.c_str(), string.Len());
}

void showMessageBox(MessageBoxType type, std::string_view title, std::string_view message, void *parent)
{
    long style = wxOK | wxCENTRE;
    switch (type)
    {
    case MessageBoxType::Warning:
        style |= wxICON_WARNING;
        break;
    case MessageBoxType::Error:
        style |= wxICON_ERROR;
        break;
    default:
        style |= wxICON_INFORMATION;
        break;
    }
    wxMessageDialog dialog((wxWindow *)parent, convert(message), convert(title), style);
    dialog.ShowModal();
}

Execution::Execution() : wxProcess(), ready(false)
{
}

bool Execution::isReady()
{
    return ready;
}

void Execution::OnTerminate(int, int)
{
    ready = true;
}

long Execution::execute(const wxString &command)
{
    return wxExecute(command, wxEXEC_ASYNC, this);
}

template <typename Dialog> DialogResult handle(Dialog &dialog, FileDialog::Handler &handler)
{
    switch (dialog.ShowModal())
    {
    case wxID_OK: {
        wxArrayString paths;
        dialog.GetPaths(paths);
        for (const auto &path : paths)
        {
            std::string_view view(path.c_str(), path.Len());
            if (!handler.handle(view))
            {
                goto error;
            }
        }
        return DialogResult::Ok;
    }
    break;
    case wxID_CANCEL:
        return DialogResult::Cancel;
    default:
        break;
    }
error:
    return DialogResult::Error;
}

DialogResult FileDialog::show(FileDialog::Handler &handler, void *parent) const
{
    int flags = 0;

    if (directories)
    {
        if (saving)
        {
            return DialogResult::Error;
        }
        else
        {
            flags = wxDD_DIR_MUST_EXIST | (multiple ? wxDD_MULTIPLE : 0);
        }

        wxDirDialog dialog((wxWindow *)parent, convert(title), convert(startDirectory), flags);
        return handle(dialog, handler);
    }
    else
    {
        if (saving)
        {
            flags = wxFD_SAVE | wxFD_OVERWRITE_PROMPT;
        }
        else
        {
            flags = wxFD_OPEN | wxFD_FILE_MUST_EXIST | (multiple ? wxFD_MULTIPLE : 0);
        }

        wxFileDialog dialog((wxWindow *)parent, convert(title), convert(startDirectory), convert(filename),
                            convert(filters), flags);
        return handle(dialog, handler);
    }
}

wxString randomString(size_t n)
{
    wxString s;
    while (s.Len() < n)
    {
        char c;
        do
        {
            c = rand() % 128;
        } while (!wxIsalnum(c));
        s.Append(c);
    }
    return s;
}

TempFile::TempFile(const wxString &ext) : path(randomString(rand() % 5 + 3)), extension(ext)
{
}

TempFile::~TempFile()
{
    std::error_code err;
    std::filesystem::remove(getPath(), err);
}

wxString TempFile::getName() const
{
    return wxString::Format(wxT("%s.%s"), path, extension);
}

std::filesystem::path TempFile::getPath() const
{
    return std::filesystem::path(getName().ToStdString());
}

wxString TempFile::OpenCommand() const
{
    const char *openCommand =
#if _WIN32
        "open";
#else
        "xdg-open";
#endif
    return wxString::Format("%s \"%s\"", openCommand, getName());
}

bool TempFile::read(wxString &s) const
{
    String string(s.ToStdString());
    if (read(string))
    {
        s = convert(string);
        return true;
    }
    return false;
}

bool TempFile::read(String &string) const
{
    std::ifstream file(getPath());
    if (file.is_open())
    {
        string.assign(std::istreambuf_iterator<char>{file}, {});
        return true;
    }
    return false;
}

bool TempFile::write(const wxString &s) const
{
    const String string(s.ToStdString());
    return write(string);
}

bool TempFile::write(const String &string) const
{
    std::ofstream file(getPath());
    if (file.is_open())
    {
        file << string;
        return true;
    }
    return false;
}

FormPanel::FormPanel(wxWindow *parent, Form &form)
    : wxPanel(parent), name(), id(wxID_HIGHEST + 1), grid(new wxFlexGridSizer(2, wxSize(8, 8)))
{
    for (auto &[n, data] : *form)
    {
        name = n;
        std::visit(*this, *data);
    }
    SetSizerAndFit(grid);
}

void FormPanel::operator()(bool &b)
{
    wxCheckBox *checkBox = new wxCheckBox(this, id, convert(name));
    checkBox->SetValue(b);
    Bind(
        wxEVT_CHECKBOX, [&b](wxCommandEvent &e) { b = e.IsChecked(); }, id++);
    grid->Add(checkBox, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL, 5);
}

void FormPanel::operator()(Number &n)
{
    std::visit(*this, n);
}

template <typename F>
bool editStringInTextEditor(const wxString &string, const wxString &ext, wxWindow *parent, F &&func)
{
    std::shared_ptr<TempFile> tempFile = std::make_shared<TempFile>(ext);
    if (!tempFile->write(string))
    {
        return false;
    }

    auto runner = shareRunAfter<Execution>([parent, tempFile, func = std::move(func)]() {
        wxWindowPtr<wxMessageDialog> dialog(new wxMessageDialog(parent, wxT("Apply changes from file?"), wxT("Apply?"),
                                                                wxYES_NO | wxICON_QUESTION | wxCENTRE));
        dialog->ShowWindowModalThenDo([dialog, tempFile, func](int retcode) {
            if (retcode != wxID_YES)
            {
                return;
            }
            wxString string;
            if (tempFile->read(string))
            {
                func(std::move(string));
            }
        });
    });

    switch (runner->execute(tempFile->OpenCommand()))
    {
    case -1:
        runner->run();
        return true;
    case 0:
        showMessageBox(MessageBoxType::Error, "Process Error", "Failed to create process", parent);
        break;
    default:
        waitUntilMessage("Wait", "Waiting for text editor to close...", runner, parent);
        return true;
    }
    return false;
}

void FormPanel::operator()(String &string)
{
    wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
    wxTextCtrl *ctrl = new wxTextCtrl(box->GetStaticBox(), id, convert(string), wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE | wxTE_BESTWRAP);
    Bind(
        wxEVT_TEXT, [&string](wxCommandEvent &e) { string = e.GetString().ToStdString(); }, id++);

    wxButton *button = new wxButton(box->GetStaticBox(), id, wxT("..."));
    Bind(
        wxEVT_BUTTON,
        [this, ctrl](wxCommandEvent &) {
            wxWindowPtr<wxTextEntryDialog> dialog(
                new wxTextEntryDialog(this, wxT("Enter file extension"), wxT("Opening Text Editor")));
            dialog->SetValue("txt");
            dialog->ShowWindowModalThenDo([this, dialog, ctrl](int retcode) {
                if (retcode != wxID_OK)
                {
                    return;
                }
                if (!editStringInTextEditor(ctrl->GetValue(), dialog->GetValue(), this,
                                            [ctrl](const wxString &value) { ctrl->SetValue(value); }))
                {
                    showMessageBox(MessageBoxType::Error, "Failed to open text editor", "Edit Failure", this);
                }
            });
        },
        id++);

    box->Add(ctrl, 9, wxEXPAND);
    box->Add(button, 1, wxEXPAND);
    grid->Add(box, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL, 5);
}

void FormPanel::operator()(StringSelection &selection)
{
    if (!selection.valid())
    {
        return;
    }

    wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
    wxArrayString choices;
    for (auto &name : selection.set)
    {
        choices.Add(convert(name));
    }
    wxComboBox *combo = new wxComboBox(box->GetStaticBox(), id, choices[selection.index], wxDefaultPosition,
                                       wxDefaultSize, choices, wxCB_READONLY);
    Bind(
        wxEVT_COMBOBOX, [&selection](wxCommandEvent &e) { selection.index = e.GetSelection(); }, id++);
    box->Add(combo, 1, wxEXPAND);
    grid->Add(box, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL, 5);
}

void FormPanel::operator()(StringMap &map)
{
    wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
    wxFlexGridSizer *sizer = new wxFlexGridSizer(2, wxSize(8, 8));
    for (auto &pair : map)
    {
        wxCheckBox *checkBox = new wxCheckBox(box->GetStaticBox(), id, convert(pair.first));
        checkBox->SetValue(pair.second);
        Bind(
            wxEVT_CHECKBOX, [&pair](wxCommandEvent &e) { pair.second = e.IsChecked(); }, id++);
        sizer->Add(checkBox, 1, wxEXPAND);
    }
    box->Add(sizer, 1, wxEXPAND);
    grid->Add(box, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL, 5);
}

void FormPanel::operator()(MultiForm &multi)
{
    wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
    wxNotebook *book = new wxNotebook(box->GetStaticBox(), id);
    Bind(
        wxEVT_NOTEBOOK_PAGE_CHANGED,
        [book, &multi](wxBookCtrlEvent &e) {
            int i = e.GetSelection();
            for (auto &[tabName, formData] : multi.tabs)
            {
                if (i == 0)
                {
                    multi.selected = tabName;
                    break;
                }
                --i;
            }
        },
        id++);
    for (auto &[tabName, formData] : multi.tabs)
    {
        FormPanel *panel = new FormPanel(book, formData);
        book->AddPage(panel, convert(tabName), tabName == multi.selected);
    }
    box->Add(book, 1, wxEXPAND);
    grid->Add(box, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL, 5);
}

FormDialog::FormDialog(wxWindow *parent, const wxString &title, Form &form) : wxDialog(parent, wxID_ANY, title)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new FormPanel(this, form), 9, wxALIGN_CENTER | wxALL, 10);

    wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton *okButton = new wxButton(this, wxID_OK, wxT("Ok"));
    buttons->Add(okButton, 1);
    wxButton *cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
    buttons->Add(cancelButton, 1, wxLEFT, 5);

    sizer->Add(buttons, 1, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizerAndFit(sizer);
}

DialogResult executeForm(std::string_view title, Form &form, void *parent)
{
    FormDialog dialog((wxWindow *)parent, convert(title), form);
    switch (dialog.ShowModal())
    {
    case wxID_OK:
        return DialogResult::Ok;
    case wxID_CANCEL:
        return DialogResult::Cancel;
    default:
        break;
    }
    return DialogResult::Error;
}

void AsyncForm::show(const std::shared_ptr<AsyncForm> &asyncForm, std::string_view title, void *parent)
{
    if (asyncForm == nullptr)
    {
        return;
    }
    wxWindowPtr<FormDialog> dialog(new FormDialog((wxWindow *)parent, convert(title), asyncForm->form));
    dialog->ShowWindowModalThenDo([asyncForm, dialog](int retcode) {
        DialogResult result = DialogResult::Error;
        switch (retcode)
        {
        case wxID_OK:
            result = DialogResult::Ok;
            break;
        case wxID_CANCEL:
            result = DialogResult::Cancel;
            break;
        default:
            break;
        }
        asyncForm->onSubmit(result);
    });
}

WaitDialog::WaitDialog(wxWindow *parent, const wxString &message, const wxString &title,
                       const std::shared_ptr<RunAfter> &r)
    : wxDialog(parent, wxID_ANY, title), timer(this), runner(r)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxStaticText(this, wxID_ANY, message));

    SetSizerAndFit(sizer);

    timer.Start(50);
}

void WaitDialog::OnTimer(wxTimerEvent &)
{
    if (runner == nullptr || runner->isReady())
    {
        EndModal(wxID_OK);
        timer.Stop();
    }
}

void waitUntilMessage(std::string_view title, std::string_view message, const std::shared_ptr<RunAfter> &runner,
                      void *parent)
{
    wxWindowPtr<WaitDialog> dialog(new WaitDialog((wxWindow *)parent, convert(message), convert(title), runner));
    dialog->ShowWindowModalThenDo([dialog, runner](int) { runner->run(); });
}

wxBEGIN_EVENT_TABLE(WaitDialog, wxDialog) EVT_TIMER(wxID_ANY, WaitDialog::OnTimer) wxEND_EVENT_TABLE();
} // namespace CanForm