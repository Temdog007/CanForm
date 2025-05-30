#pragma once

#include <canform.hpp>
#include <cstring>
#include <filesystem>
#include <glibmm/ustring.h>
#include <gtkmm.h>
#include <memory>

namespace CanForm
{
extern Glib::ustring convert(const std::string &s);
extern Glib::ustring convert(std::string_view);
extern std::string convert(const Glib::ustring &);
extern std::string_view toView(const Glib::ustring &);

constexpr const char *getIconName(MessageBoxType type) noexcept
{
    switch (type)
    {
    case MessageBoxType::Warning:
        return "dialog-warning";
    case MessageBoxType::Error:
        return "dialog-error";
    default:
        break;
    }
    return "dialog-information";
}

struct IBufferSetter
{
    virtual ~IBufferSetter()
    {
    }
    virtual void set(Glib::ustring &&) = 0;
};

template <typename F> struct BufferSetter : public IBufferSetter
{
    F func;
    BufferSetter(F &&f) : func(std::move(f))
    {
    }
    virtual ~BufferSetter()
    {
    }
    virtual void set(Glib::ustring &&s) override
    {
        func(std::move(s));
    }
};

class TempFile
{
  private:
    Glib::ustring filename;
    Glib::ustring extension;
    mutable std::filesystem::file_time_type timePoint;

  public:
    TempFile(const Glib::ustring &ext = "txt");

    Glib::ustring getName() const;
    std::filesystem::path getPath() const;

    bool changed() const;

    bool read(std::string &string, bool updatedTimePoint) const;
    bool write(const std::string &string) const;

    bool read(Glib::ustring &, bool updatedTimePoint) const;
    bool write(const Glib::ustring &) const;

    bool open() const;

    static bool openFile(std::string_view);
    static bool openTempDirectory();

    static sigc::connection syncBuffer(std::weak_ptr<TempFile> ptr, const Glib::RefPtr<Gtk::TextBuffer> &buffer);
};

struct SyncButton : public Gtk::Button
{
    SyncButton(const Glib::ustring &, const Glib::RefPtr<Gtk::TextBuffer> &);
    virtual ~SyncButton()
    {
    }
};

} // namespace CanForm