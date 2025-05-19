#include <canform.hpp>
#include <wx/busyinfo.h>
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

} // namespace CanForm