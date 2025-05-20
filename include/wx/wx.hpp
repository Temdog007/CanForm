#pragma once

#include <canform.hpp>

#include <filesystem>
#include <string_view>

#include <wx/numformatter.h>
#include <wx/process.h>
#include <wx/wx.h>

namespace CanForm
{
extern wxString convert(std::string_view);
extern std::string_view toView(const wxString &) noexcept;

class TempFile
{
  private:
    wxString path;
    wxString extension;

  public:
    TempFile(const wxString &ext);
    ~TempFile();

    wxString getName() const;
    std::filesystem::path getPath() const;

    bool read(String &string) const;
    bool write(const String &string) const;

    bool read(wxString &) const;
    bool write(const wxString &) const;

    wxString OpenCommand() const;
};

class Execution : public wxProcess, public RunAfter
{
  private:
    bool ready;

  public:
    Execution();
    Execution(Execution &&) noexcept = default;
    virtual ~Execution()
    {
    }

    virtual bool isReady() override;

    virtual void OnTerminate(int, int) override;

    long execute(const wxString &);
};

template <typename T> class NumberControl : public wxPanel
{
  private:
    static_assert(std::is_arithmetic<T>::value);

    wxTextCtrl *text;
    T &value;

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

  public:
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
};

class FormDialog : public wxDialog
{
  private:
    std::string_view name;
    int id;
    wxFlexGridSizer *grid;

    void OnOk(wxCommandEvent &);
    void OnCancel(wxCommandEvent &);

  public:
    FormDialog(wxWindow *parent, wxWindowID id, const wxString &title, Form &form);

    virtual ~FormDialog()
    {
    }

    void operator()(bool &b);

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

    void operator()(Number &n);

    void operator()(String &string);

    void operator()(StringSelection &selection);

    void operator()(StringMap &map);

    DECLARE_EVENT_TABLE()
};

class WaitDialog : public wxDialog
{
  private:
    wxTimer timer;
    std::shared_ptr<RunAfter> runner;

    void OnTimer(wxTimerEvent &);

  public:
    WaitDialog(wxWindow *, const wxString &message, const wxString &title, const std::shared_ptr<RunAfter> &);
    virtual ~WaitDialog()
    {
    }

    DECLARE_EVENT_TABLE()
};

} // namespace CanForm