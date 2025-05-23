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
} // namespace CanForm