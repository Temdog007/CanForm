#pragma once

#include <canform.hpp>
#include <glibmm/ustring.h>

namespace CanForm
{
extern Glib::ustring convert(const std::string &s);
extern Glib::ustring convert(std::string_view);
extern std::string_view convert(const Glib::ustring &);
extern Glib::ustring randomString(size_t min, size_t max);
extern Glib::ustring randomString(size_t n);

class TempFile
{
  private:
    Glib::ustring path;
    Glib::ustring extension;

  public:
    TempFile(const Glib::ustring &ext);
    ~TempFile();

    Glib::ustring getName() const;
    std::filesystem::path getPath() const;

    bool read(String &string) const;
    bool write(const String &string) const;

    bool read(Glib::ustring &) const;
    bool write(const Glib::ustring &) const;

    bool spawnEditor() const;
};
} // namespace CanForm