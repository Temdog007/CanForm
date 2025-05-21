#pragma once

#include <canform.hpp>

#include <filesystem>
#include <optional>
#include <string_view>

#include <wx/affinematrix2d.h>
#include <wx/notebook.h>
#include <wx/numformatter.h>
#include <wx/process.h>
#include <wx/valnum.h>
#include <wx/wx.h>

namespace CanForm
{
extern wxString convert(std::string_view);
extern std::string_view toView(const wxString &) noexcept;
extern wxString randomString(size_t min, size_t max);
extern wxString randomString(size_t n);

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

    void OnValueUpdated(wxCommandEvent &e)
    {
        if constexpr (std::is_floating_point<T>::value)
        {
            value = wxAtof(e.GetString());
        }
        else
        {
            const String string(e.GetString().ToStdString());
            char *end;
            if constexpr (std::is_signed_v<T>)
            {
                const int64_t u = std::strtoll(string.c_str(), &end, 10);
                if (string.data() + string.size() != end)
                {
                    return;
                }
                value = std::clamp(static_cast<T>(u), std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            }
            else
            {
                const uint64_t u = std::strtoull(string.c_str(), &end, 10);
                if (string.data() + string.size() != end)
                {
                    return;
                }
                value = std::clamp(static_cast<T>(u), std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            }
        }
        updateText();
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
        Bind(wxEVT_TEXT, &NumberControl::OnValueUpdated, this, id);
        updateText();

        sizer->Add(left, 1, wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
        sizer->Add(text, 8, wxALIGN_CENTRE_VERTICAL);
        sizer->Add(right, 1, wxALIGN_CENTRE_VERTICAL);

        SetSizerAndFit(sizer);
    }
    virtual ~NumberControl()
    {
    }
};

class FormPanel : public wxPanel
{
  private:
    std::string_view name;
    int id;
    wxFlexGridSizer *grid;

  public:
    FormPanel(wxWindow *parent, Form &form);

    virtual ~FormPanel()
    {
    }

    void operator()(bool &b);

    template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> void operator()(T &t)
    {
        wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, convert(name));
        NumberControl<T> *ctrl = new NumberControl<T>(box->GetStaticBox(), id++, t);
        box->Add(ctrl, 1, wxEXPAND);
        grid->Add(box, 1, wxEXPAND, 5);
    }

    void operator()(Number &n);

    void operator()(String &string);

    void operator()(StringSelection &selection);

    void operator()(StringMap &map);

    void operator()(MultiForm &);
};

class FormDialog : public wxDialog
{
  private:
    void OnOk(wxCommandEvent &);
    void OnCancel(wxCommandEvent &);

  public:
    FormDialog(wxWindow *, const wxString &, Form &);

    virtual ~FormDialog()
    {
    }
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

extern wxNotebook *gNotebook;

class NotebookPage : public wxPanel
{
  private:
    void OnPaint(wxPaintEvent &);
    void OnMouse(wxMouseEvent &);
    void OnMenu(wxCommandEvent &);
    void OnCaptureLost(wxMouseCaptureLostEvent &);

    void close();
    void reset();

    RenderAtoms atoms;
    Rectangle viewRect;
    wxPoint lastMouse;

    struct CaptureState
    {
        wxWindow &window;
        CaptureState(wxWindow &);
        ~CaptureState();
    };
    std::optional<CaptureState> captureState;

    friend bool CanForm::getCanvasAtoms(std::string_view, RenderAtomsUser &, bool);

  public:
    NotebookPage(wxWindow *);
    virtual ~NotebookPage()
    {
    }

    DECLARE_EVENT_TABLE()
};

} // namespace CanForm