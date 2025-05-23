#pragma once

#include "../canvas.hpp"
#include "../form.hpp"

namespace CanForm
{
extern Form makeForm();
extern void printForm(const Form &, DialogResult, void *parent = nullptr);

struct RandomRender
{
    std::pair<double, double> size;

    template <typename... Args> RandomRender(Args &&...args) noexcept : size(std::forward<Args>(args)...)
    {
    }

    void randomPosition(double &x, double &y);
    void operator()(RenderStyle &style);
    void operator()(CanFormRectangle &r);
    void operator()(CanFormEllipse &r);
    void operator()(RoundedRectangle &r);

    void operator()(Text &t);

    RenderAtom operator()();
};
} // namespace CanForm