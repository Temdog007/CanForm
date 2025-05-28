#pragma once

#include <limits>
#include <optional>

namespace CanForm
{
template <typename T> class Range
{
    static_assert(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>);

  private:
    T value;
    T min;
    T max;

    constexpr Range(T v, T mi, T ma) noexcept : value(v), min(mi), max(ma)
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