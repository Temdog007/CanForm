#include <canform.hpp>
#include <wx/busyinfo.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace CanForm
{
wxString convert(std::string_view string)
{
    return wxString::FromUTF8(string.data(), string.size());
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

void waitUntilMessage(std::string_view title, std::string_view message, Done &checker, void *parent)
{
    wxWindowDisabler disableAll;
    wxBusyInfo wait(wxBusyInfoFlags().Parent((wxWindow *)parent).Title(convert(title)).Text(convert(message)));
    for (size_t i = 0; !checker.isDone(); ++i)
    {
        if (i % 1000 == 0)
        {
            wxTheApp->Yield();
        }
    }
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

struct FormDialog : public wxDialog
{
    std::string_view name;
    int id;
    wxFlexGridSizer *grid;

    FormDialog(wxWindow *parent, wxWindowID id, const wxString &title, Form &form)
        : wxDialog(parent, id, title), name(), id(wxID_HIGHEST + 1), grid(new wxFlexGridSizer(2, wxSize(2, 2)))
    {
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

        for (auto &[n, data] : form)
        {
            name = n;
            std::visit(*this, data);
        }
        sizer->Add(grid, 0, wxEXPAND, 10);

        wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
        wxButton *cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
        wxButton *okButton = new wxButton(this, wxID_OK, wxT("Ok"));
        buttons->Add(okButton, 1);
        buttons->Add(cancelButton, 1, wxLEFT, 5);

        sizer->AddSpacer(10);
        sizer->Add(buttons, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

        SetSizer(sizer);
    }
    virtual ~FormDialog()
    {
    }

    void operator()(bool &b)
    {
        wxCheckBox *checkBox = new wxCheckBox(this, id, convert(name));
        Bind(
            wxEVT_CHECKBOX, [&b](wxCommandEvent &e) { b = e.IsChecked(); }, id++);
        checkBox->SetValue(b);
        grid->Add(checkBox);
    }

    void operator()(long &l)
    {
        wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
        wxSpinCtrl *ctrl = new wxSpinCtrl(box->GetStaticBox(), id, wxString::Format(wxT("%ld"), l));
        Bind(
            wxEVT_SPINCTRL, [&l](wxSpinEvent &e) { l = e.GetInt(); }, id++);
        box->Add(ctrl);
        grid->Add(box, 1, wxEXPAND, 5);
    }

    void operator()(String &string)
    {
        wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
        wxTextCtrl *ctrl = new wxTextCtrl(box->GetStaticBox(), id, convert(string));
        Bind(
            wxEVT_TEXT, [&string](wxCommandEvent &e) { string = e.GetString().ToStdString(); }, id++);
        box->Add(ctrl);
        grid->Add(box, 1, wxEXPAND, 5);
    }

    void operator()(StringSet &)
    {
    }
    void operator()(StringMap &)
    {
    }

    void OnOk(wxCommandEvent &)
    {
        EndModal(wxID_OK);
    }
    void OnCancel(wxCommandEvent &)
    {
        EndModal(wxID_CANCEL);
    }

    DECLARE_EVENT_TABLE()
};

DialogResult executeForm(std::string_view title, Form &form, void *parent)
{
    FormDialog dialog((wxWindow *)parent, wxID_ANY, convert(title), form);
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

wxBEGIN_EVENT_TABLE(FormDialog, wxDialog) EVT_MENU(wxID_OK, FormDialog::OnOk)
    EVT_MENU(wxID_CANCEL, FormDialog::OnCancel) wxEND_EVENT_TABLE()

} // namespace CanForm