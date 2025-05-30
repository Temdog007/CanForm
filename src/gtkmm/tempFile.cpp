#include <fstream>
#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>

#if __WIN32
#include <Windows.h>
#endif

namespace CanForm
{
TempFile::TempFile(const Glib::ustring &ext) : filename(), extension(ext), timePoint()
{
    randomString(filename, 3, 8);
}

Glib::ustring TempFile::getName() const
{
    return Glib::ustring::sprintf("%s.%s", filename, extension);
}

std::filesystem::path TempFile::getPath() const
{
    std::error_code err;
    std::filesystem::path path = std::filesystem::temp_directory_path(err);
    path /= convert(getName());
    return path;
}

bool TempFile::read(Glib::ustring &s, bool updateTimePoint) const
{
    std::string string(convert(s));
    if (read(string, updateTimePoint))
    {
        s = convert(string);
        return true;
    }
    return false;
}

bool TempFile::read(std::string &string, bool updateTimePoint) const
{
    const auto path = getPath();
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            std::error_code err;
            const auto fileSize = std::filesystem::file_size(path, err);
            string.resize(fileSize, '\0');
            file.read(string.data(), fileSize);
            goto storeWrite;
        }
    }
    return false;
storeWrite:
    if (updateTimePoint)
    {
        std::error_code err;
        timePoint = std::filesystem::last_write_time(path, err);
        if (err)
        {
            return false;
        }
    }
    return true;
}

bool TempFile::write(const Glib::ustring &s) const
{
    const std::string string(s);
    return write(string);
}

bool TempFile::write(const std::string &string) const
{
    const auto path = getPath();
    {
        std::ofstream file(path);
        if (file.is_open())
        {
            file << string;
            goto storeWrite;
        }
    }
    return false;
storeWrite:
    std::error_code err;
    timePoint = std::filesystem::last_write_time(path, err);
    if (err)
    {
        return false;
    }
    return true;
}

bool TempFile::changed() const
{
    std::error_code err;
    return timePoint != std::filesystem::last_write_time(getPath(), err);
}

bool TempFile::openFile(std::string_view filePath)
{
    try
    {
        std::error_code err;
        auto path = std::filesystem::temp_directory_path(err);
        if (!filePath.empty())
        {
            path /= filePath;
        }
        auto pathString = path.string();
        pathString.insert(0, "file://");
#if __WIN32
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (hr == RPC_E_CHANGED_MODE)
        {
            hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        }
        if (hr != S_FALSE && FAILED(hr))
        {
            return false;
        }
        HINSTANCE rc = ShellExecute(nullptr, "open", pathString.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        CoUninitialize();
        return rc <= (HINSTANCE)32;
#else
        std::vector<std::string> argv;
        argv.emplace_back("xdg-open");
        argv.emplace_back(std::move(pathString));
        Glib::spawn_async("", argv,
                          Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_STDOUT_TO_DEV_NULL | Glib::SPAWN_STDERR_TO_DEV_NULL);
#endif
        return true;
    }
    catch (const std::exception &e)
    {
        showMessageBox(MessageBoxType::Error, "Failed to open text editor", e.what());
        return false;
    }
}

bool TempFile::open() const
{
    return openFile(convert(getName()));
}

bool TempFile::openTempDirectory()
{
    return openFile("");
}

} // namespace CanForm