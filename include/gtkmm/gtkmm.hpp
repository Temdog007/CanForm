#pragma once

#include <canform.hpp>
#include <glibmm/ustring.h>

namespace CanForm
{
extern Glib::ustring convert(std::string_view);
extern std::string_view convert(const Glib::ustring &);
} // namespace CanForm