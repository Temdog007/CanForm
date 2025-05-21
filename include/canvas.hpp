#pragma once

#include "tie.hpp"
#include "types.hpp"

namespace CanForm
{
struct Rectangle
{
    double x, y, w, h;
    constexpr Rectangle() noexcept : x(0), y(0), w(0), h(0)
    {
    }
    constexpr Rectangle(const Rectangle &) noexcept = default;
    constexpr Rectangle(Rectangle &&) noexcept = default;

    constexpr Rectangle &operator=(const Rectangle &) noexcept = default;
    constexpr Rectangle &operator=(Rectangle &&) noexcept = default;

    constexpr Rectangle &expand(double dx, double dy) noexcept
    {
        x -= dx;
        y -= dy;
        w += dx * 2.0;
        h += dy * 2.0;
        return *this;
    }

    constexpr std::pair<double, double> center() const noexcept
    {
        return std::make_pair(x + w * 0.5, y + h * 0.5);
    }

    constexpr Rectangle &expand(double d) noexcept
    {
        return expand(d, d);
    }

    constexpr auto makeTie() const noexcept
    {
        return std::tie(x, y, w, h);
    }

    TIE(Rectangle)
};

struct RoundedRectangle
{
    Rectangle rectangle;
    double radius;

    constexpr RoundedRectangle() noexcept : rectangle(), radius(0)
    {
    }
    constexpr RoundedRectangle(const RoundedRectangle &) noexcept = default;
    constexpr RoundedRectangle(RoundedRectangle &&) noexcept = default;

    constexpr RoundedRectangle &operator=(const RoundedRectangle &) noexcept = default;
    constexpr RoundedRectangle &operator=(RoundedRectangle &&) noexcept = default;

    constexpr auto makeTie() const noexcept
    {
        return std::tie(rectangle, radius);
    }

    TIE(RoundedRectangle)
};

struct Ellipse
{
    double x, y, w, h;
    constexpr Ellipse() noexcept : x(0), y(0), w(0), h(0)
    {
    }
    constexpr Ellipse(const Ellipse &) noexcept = default;
    constexpr Ellipse(Ellipse &&) noexcept = default;

    constexpr Ellipse &operator=(const Ellipse &) noexcept = default;
    constexpr Ellipse &operator=(Ellipse &&) noexcept = default;

    constexpr auto makeTie() const noexcept
    {
        return std::tie(x, y, w, h);
    }

    TIE(Ellipse)
};

struct Text
{
    String string;
    double x, y;

    Text() : string(), x(0), y(0)
    {
    }
    Text(const Text &) = default;
    Text(Text &&) noexcept = default;

    Text &operator=(const Text &) = default;
    Text &operator=(Text &&) = default;

    constexpr auto makeTie() const noexcept
    {
        return std::tie(string, x, y);
    }

    TIE(Text)
};

using RenderType = std::variant<Rectangle, RoundedRectangle, Ellipse, Text>;

struct Color
{
    uint8_t red, green, blue, alpha;
    constexpr Color() noexcept : red(255u), green(255u), blue(255u), alpha(255u)
    {
    }
    constexpr Color(const Color &) noexcept = default;
    constexpr Color(Color &&) noexcept = default;

    constexpr Color &operator=(const Color &) noexcept = default;
    constexpr Color &operator=(Color &&) noexcept = default;

    constexpr auto makeTie() const noexcept
    {
        return std::tie(red, green, blue, alpha);
    }

    TIE(Color)
};

struct RenderStyle
{
    Color color;
    bool fill;

    constexpr RenderStyle() noexcept : color(), fill(false)
    {
    }
    constexpr RenderStyle(const RenderStyle &) noexcept = default;
    constexpr RenderStyle(RenderStyle &&) noexcept = default;

    constexpr RenderStyle &operator=(const RenderStyle &) noexcept = default;
    constexpr RenderStyle &operator=(RenderStyle &&) noexcept = default;

    constexpr auto makeTie() const noexcept
    {
        return std::tie(color, fill);
    }

    TIE(RenderStyle)
};

struct RenderAtom
{
    RenderType renderType;
    RenderStyle style;

    RenderAtom() noexcept : renderType(), style()
    {
    }
    RenderAtom(const RenderAtom &) noexcept = default;
    RenderAtom(RenderAtom &&) noexcept = default;

    RenderAtom &operator=(const RenderAtom &) noexcept = default;
    RenderAtom &operator=(RenderAtom &&) noexcept = default;

    constexpr auto makeTie() const noexcept
    {
        return std::tie(renderType, style);
    }

    TIE(RenderAtom)
};

using RenderAtoms = std::pmr::vector<RenderAtom>;

struct RenderAtomsUser
{
    virtual ~RenderAtomsUser()
    {
    }

    virtual void use(RenderAtoms &, Rectangle &) = 0;
};

extern bool getCanvasAtoms(std::string_view, RenderAtomsUser &, bool createdIfNeeded = false);

} // namespace CanForm