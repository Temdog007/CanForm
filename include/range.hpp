#pragma once

#include <algorithm>
#include <limits>
#include <optional>

namespace CanForm
{
struct IRange
{
    virtual void setFromDouble(double) = 0;
};

template <typename T> class Range : public IRange
{
    static_assert(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>);

  private:
    T value;
    T min;
    T max;

    template <typename U> friend class Range;

    constexpr Range(T v, T mi, T ma) noexcept
        : value(std::clamp(v, mi, ma)), min(std::min(mi, ma)), max(std::max(mi, ma))
    {
    }

  public:
    constexpr Range(T t) noexcept : value(t), min(), max()
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            min = std::numeric_limits<T>::max() * -1.0;
            max = std::numeric_limits<T>::max();
        }
        else
        {
            min = std::numeric_limits<T>::min();
            max = std::numeric_limits<T>::max();
        }
    }
    constexpr Range(const Range &) noexcept = default;
    constexpr Range(Range &&) noexcept = default;

    constexpr Range &operator=(const Range &) noexcept = default;
    constexpr Range &operator=(Range &&) noexcept = default;

    constexpr Range &operator=(T t) noexcept
    {
        setValue(t);
        return *this;
    }

    template <typename U, std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, bool> = true>
    Range &operator=(U u) noexcept
    {
        Range<U> r(u);
        min = static_cast<T>(r.min);
        max = static_cast<T>(r.max);
        value = std::clamp(static_cast<T>(u), min, max);
        return *this;
    }

    virtual void setFromDouble(double d) override
    {
        *this = d;
    }

    constexpr T getValue() const noexcept
    {
        return value;
    }
    constexpr bool setValue(T t) noexcept
    {
        if (inRange(t))
        {
            value = t;
            return true;
        }
        else
        {
            return false;
        }
    }

    constexpr std::pair<T, T> getMinMax() const noexcept
    {
        return std::pair<T, T>(min, max);
    }

    constexpr bool inRange(T t) const noexcept
    {
        return min <= t && t <= max;
    }

    constexpr T operator*() const noexcept
    {
        return getValue();
    }

    constexpr const T *operator->() const noexcept
    {
        return &value;
    }
    T *operator->() noexcept
    {
        return &value;
    }

    static std::optional<Range<T>> create(T value, T min, T max) noexcept
    {
        if (min <= value && value <= max)
        {
            return Range(value, min, max);
        }
        else
        {
            return std::nullopt;
        }
    }
};
} // namespace CanForm