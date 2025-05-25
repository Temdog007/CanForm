#pragma once

#include <canform.hpp>
#include <filesystem>
#include <glibmm/ustring.h>
#include <gtkmm/drawingarea.h>
#include <memory>
#include <unordered_set>

namespace CanForm
{
extern Glib::ustring convert(const std::string &s);
extern Glib::ustring convert(std::string_view);
extern std::string convert(const Glib::ustring &);
extern std::string_view toView(const Glib::ustring &);

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
    Glib::ustring path;
    Glib::ustring extension;
    std::filesystem::file_time_type timePoint;

  public:
    TempFile(const Glib::ustring &ext = "txt");
    ~TempFile();

    Glib::ustring getName() const;
    std::filesystem::path getPath() const;

    bool changed() const;

    bool read(String &string) const;
    bool write(const String &string);

    bool read(Glib::ustring &) const;
    bool write(const Glib::ustring &);

    void open() const;

    template <typename B> static void syncBuffer(std::weak_ptr<TempFile> ptr, B buffer)
    {
        auto slot = [ptr, buffer]() {
            auto file = ptr.lock();
            if (file == nullptr)
            {
                return false;
            }
            if (!file->changed())
            {
                return true;
            }
            Glib::ustring string;
            if (!file->read(string))
            {
                return false;
            }
            if (buffer)
            {
                buffer->set_text(string);
                return true;
            }
            return false;
        };
        Glib::signal_timeout().connect(slot, 100);
    }
};

static inline TimePoint now() noexcept
{
    return std::chrono::system_clock::now();
}
} // namespace CanForm