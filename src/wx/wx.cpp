#include <canform.hpp>
#include <wx/busyinfo.h>
#include <wx/numformatter.h>
#include <wx/spinctrl.h>
#include <wx/valnum.h>
#include <wx/windowptr.h>
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

template <typename T> struct NumberControl : public wxPanel
{
    static_assert(std::is_arithmetic<T>::value);

    wxTextCtrl *text;
    T &value;

    NumberControl(wxWindow *parent, wxWindowID id, T &v) : wxPanel(parent), text(nullptr), value(v)
    {
        wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

        wxButton *left = new wxButton(this, wxID_BACKWARD, wxT("←"));
        Bind(wxEVT_BUTTON, &NumberControl::OnBackward, this, wxID_BACKWARD);

        wxButton *right = new wxButton(this, wxID_FORWARD, wxT("→"));
        Bind(wxEVT_BUTTON, &NumberControl::OnForward, this, wxID_FORWARD);

        if constexpr (std::is_floating_point<T>::value)
        {
            text = new wxTextCtrl(this, id, wxT(""), wxDefaultPosition, wxDefaultSize, 0,
                                  wxFloatingPointValidator<T>(6, &value));
        }
        else
        {
            text =
                new wxTextCtrl(this, id, wxT(""), wxDefaultPosition, wxDefaultSize, 0, wxIntegerValidator<T>(&value));
        }
        updateText();

        sizer->Add(left, 1, wxALIGN_LEFT);
        sizer->Add(text, 8, wxALIGN_CENTER);
        sizer->Add(right, 1);

        SetSizerAndFit(sizer);
    }
    virtual ~NumberControl()
    {
    }

    void OnBackward(wxCommandEvent &)
    {
        --value;
        updateText();
    }
    void OnForward(wxCommandEvent &)
    {
        ++value;
        updateText();
    }

  private:
    void updateText()
    {
        wxString string;
        if constexpr (std::is_floating_point<T>::value)
        {
            string = wxString::Format(wxT("%f"), value);
        }
        else if constexpr (std::is_signed_v<T>)
        {
            string =
                wxNumberFormatter::ToString(static_cast<wxLongLong_t>(value), wxNumberFormatter::Style::Style_None);
        }
        else
        {
            string =
                wxNumberFormatter::ToString(static_cast<wxULongLong_t>(value), wxNumberFormatter::Style::Style_None);
        }
        text->ChangeValue(string);
    }
};

struct FormDialog : public wxDialog
{
    std::string_view name;
    int id;
    wxFlexGridSizer *grid;

    FormDialog(wxWindow *parent, wxWindowID id, const wxString &title, Form &form)
        : wxDialog(parent, id, title), name(), id(wxID_HIGHEST + 1), grid(new wxFlexGridSizer(2, wxSize(8, 8)))
    {
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

        for (auto &[n, data] : form)
        {
            name = n;
            std::visit(*this, data);
        }
        sizer->Add(grid, 9, wxALIGN_CENTER | wxALL, 10);

        wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
        wxButton *okButton = new wxButton(this, wxID_OK, wxT("Ok"));
        buttons->Add(okButton, 1);
        wxButton *cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
        buttons->Add(cancelButton, 1, wxLEFT, 5);

        sizer->Add(buttons, 1, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);

        SetSizerAndFit(sizer);
    }
    virtual ~FormDialog()
    {
    }

    void operator()(bool &b)
    {
        wxCheckBox *checkBox = new wxCheckBox(this, id, convert(name));
        checkBox->SetValue(b);
        Bind(
            wxEVT_CHECKBOX, [&b](wxCommandEvent &e) { b = e.IsChecked(); }, id++);
        grid->Add(checkBox, 1, wxEXPAND, 5);
    }

    template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> void operator()(T &t)
    {
        wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
        NumberControl<T> *ctrl = new NumberControl<T>(box->GetStaticBox(), id, t);
        Bind(
            wxEVT_TEXT_ENTER,
            [&t](wxCommandEvent &e) {
                if constexpr (std::is_floating_point<T>::value)
                {
                    t = wxAtof(e.GetString());
                }
                else
                {
                    t = wxAtol(e.GetString());
                }
            },
            id++);
        box->Add(ctrl, 1, wxEXPAND);
        grid->Add(box, 1, wxEXPAND, 5);
    }

    void operator()(Number &n)
    {
        std::visit(*this, n);
    }

    void operator()(String &string)
    {
        wxStaticBoxSizer *box = new wxStaticBoxSizer(wxHORIZONTAL, this, convert(name));
        wxTextCtrl *ctrl = new wxTextCtrl(box->GetStaticBox(), id, convert(string), wxDefaultPosition, wxDefaultSize,
                                          wxTE_MULTILINE | wxTE_BESTWRAP);
        Bind(
            wxEVT_TEXT, [&string](wxCommandEvent &e) { string = e.GetString().ToStdString(); }, id++);
        box->Add(ctrl, 1, wxEXPAND);
        grid->Add(box, 1, wxEXPAND, 5);
    }

    void operator()(StringSelection &selection)
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
        grid->Add(box, 1, wxEXPAND, 5);
    }

    void operator()(StringMap &map)
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
        grid->Add(box, 1, wxEXPAND, 5);
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

void AsyncForm::show(const std::shared_ptr<AsyncForm> &asyncForm, std::string_view title, void *parent)
{
    if (asyncForm == nullptr)
    {
        return;
    }
    wxWindowPtr<FormDialog> dialog(new FormDialog((wxWindow *)parent, wxID_ANY, convert(title), asyncForm->form));
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

wxBEGIN_EVENT_TABLE(FormDialog, wxDialog) EVT_MENU(wxID_OK, FormDialog::OnOk)
    EVT_MENU(wxID_CANCEL, FormDialog::OnCancel) wxEND_EVENT_TABLE()

} // namespace CanForm