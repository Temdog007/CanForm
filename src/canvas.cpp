#include <canvas.hpp>

namespace CanForm
{
std::pair<double, double> getPosition(const RenderType &rt) noexcept
{
    struct Visitor
    {
        std::pair<double, double> operator()(const Rectangle &r) const noexcept
        {
            return std::make_pair(r.x, r.y);
        }
        std::pair<double, double> operator()(const RoundedRectangle &r) const noexcept
        {
            return operator()(r.rectangle);
        }
        std::pair<double, double> operator()(const Ellipse &r) const noexcept
        {
            return std::make_pair(r.x, r.y);
        }
        std::pair<double, double> operator()(const Text &r) const noexcept
        {
            return std::make_pair(r.x, r.y);
        }
    };
    return std::visit(Visitor(), rt);
}

Rectangle getRectangle(const RenderType &rt) noexcept
{
    struct Visitor
    {
        Rectangle operator()(const Rectangle &r) const noexcept
        {
            return r;
        }
        Rectangle operator()(const RoundedRectangle &r) const noexcept
        {
            return r.rectangle;
        }
        Rectangle operator()(const Ellipse &e) const noexcept
        {
            Rectangle r;
            r.x = e.x;
            r.y = e.y;
            r.w = e.w;
            r.h = e.h;
            return r;
        }
        Rectangle operator()(const Text &r) const noexcept
        {
            return getTextBounds(r.string);
        }
    };
    return std::visit(Visitor(), rt);
}

void setPosition(RenderType &rt, double x, double y) noexcept
{
    struct Visitor
    {
        double x, y;
        constexpr Visitor(double x, double y) noexcept : x(x), y(y)
        {
        }
        void operator()(Rectangle &r) const noexcept
        {
            r.x = x;
            r.y = y;
        }
        void operator()(RoundedRectangle &r) const noexcept
        {
            operator()(r.rectangle);
        }
        void operator()(Ellipse &e) const noexcept
        {
            e.x = x;
            e.y = y;
        }
        void operator()(Text &t) const noexcept
        {
            t.x = x;
            t.y = y;
        }
    };
    return std::visit(Visitor(x, y), rt);
}
} // namespace CanForm