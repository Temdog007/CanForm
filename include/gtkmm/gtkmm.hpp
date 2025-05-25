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
    Glib::ustring filename;
    Glib::ustring extension;
    mutable std::filesystem::file_time_type timePoint;

  public:
    TempFile(const Glib::ustring &ext = "txt");
    ~TempFile();

    Glib::ustring getName() const;
    std::filesystem::path getPath() const;

    bool changed() const;

    bool read(String &string, bool updatedTimePoint) const;
    bool write(const String &string) const;

    bool read(Glib::ustring &, bool updatedTimePoint) const;
    bool write(const Glib::ustring &) const;

    bool open() const;

    template <typename B> static auto syncBuffer(std::weak_ptr<TempFile> ptr, B buffer)
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
            if (!file->read(string, true))
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
        return Glib::signal_timeout().connect(slot, 100);
    }
};

struct SyncButton : public Gtk::Button
{
    template <typename B> SyncButton(B buffer) : Gtk::Button("Sync to File?")
    {
        signal_clicked().connect([this, buffer]() {
            auto parent = get_parent();
            if (parent == nullptr)
            {
                return;
            }
            parent->remove(*this);

            std::shared_ptr<TempFile> tempFile = std::make_shared<TempFile>();
            tempFile->write(buffer->get_text());

            Gtk::Frame *frame = Gtk::manage(new Gtk::Frame(convert(tempFile->getPath().string())));
            parent->add(*frame);

            Gtk::Button *button = Gtk::manage(new Gtk::Button("Open File"));
            button->signal_clicked().connect([tempFile]() { tempFile->open(); });
            frame->add(*button);

            auto connection = TempFile::syncBuffer(tempFile, buffer);
            frame->signal_hide().connect([connection]() mutable { connection.disconnect(); });

            parent->show_all_children();
        });
    }
};

static inline TimePoint now() noexcept
{
    return std::chrono::system_clock::now();
}
} // namespace CanForm