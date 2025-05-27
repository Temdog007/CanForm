#pragma once

#include <canform.hpp>
#include <cstring>
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
    return "gtk-dialog-info";
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
                buffer->set_text(string.make_valid());
                return true;
            }
            return false;
        };
        return Glib::signal_timeout().connect(slot, 100);
    }
};

struct SyncButton : public Gtk::Button
{
    template <typename B> SyncButton(const Glib::ustring &s, B buffer) : Gtk::Button("Sync to File?")
    {
        signal_clicked().connect([this, s, buffer]() {
            auto parent = get_parent();
            if (parent == nullptr)
            {
                return;
            }
            parent->remove(*this);

            std::shared_ptr<TempFile> tempFile = std::make_shared<TempFile>();
            tempFile->write(buffer->get_text());

            Gtk::Frame *frame = Gtk::make_managed<Gtk::Frame>(convert(tempFile->getPath().string()));
            parent->add(*frame);

            Gtk::HBox *hBox = Gtk::make_managed<Gtk::HBox>();

            Gtk::Button *button = Gtk::make_managed<Gtk::Button>("Open File");
            button->signal_clicked().connect([tempFile]() { tempFile->open(); });
            hBox->add(*button);

            button = Gtk::make_managed<Gtk::Button>("Open Directory");
            button->signal_clicked().connect([]() { TempFile::openTempDirectory(); });
            hBox->add(*button);

            frame->add(*hBox);

            auto connection = TempFile::syncBuffer(tempFile, buffer);
            frame->signal_hide().connect([connection]() mutable { connection.disconnect(); });

            parent->show_all_children();
        });
    }
};

} // namespace CanForm