#pragma once

#include "../form.hpp"

namespace CanForm
{
extern Form makeForm();
extern void printForm(const Form &, DialogResult, void *parent = nullptr);
} // namespace CanForm