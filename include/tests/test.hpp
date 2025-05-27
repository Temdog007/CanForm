#pragma once

#include "../form.hpp"

namespace CanForm
{
extern Form makeForm();
extern void printForm(const Form &, void *parent = nullptr);

template <typename T> T random() noexcept
{
    if constexpr (std::is_floating_point_v<T>)
    {
        return ((T)rand() / RAND_MAX);
    }
    else
    {
        return (T)((double)std::numeric_limits<T>::min() + (double)(std::numeric_limits<T>::max()) -
                   (double)(std::numeric_limits<T>::min()) * random<double>());
    }
}

inline StringSet randomSet(size_t n, size_t min, size_t max)
{
    StringSet set;
    for (size_t i = 0; i < n; ++i)
    {
        set.emplace(randomString(min, max));
    }
    return set;
}

template <typename T> inline void addForm(Form &form, String &&s, T &&t)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        t = random<T>();
        form[std::move(s)] = std::move(t);
    }
    else
    {
    }
}

template <> inline void addForm<bool>(Form &form, String &&s, bool &&b)
{
    b = rand() % 2 == 0;
    form[std::move(s)] = std::move(b);
}

template <> inline void addForm<String>(Form &form, String &&s, String &&value)
{
    value = randomString(3, 8);
    if (rand() % 2 == 0)
    {
        form[std::move(s)] = std::move(value);
    }
    else
    {
        ComplexString c;
        c.string = std::move(value);
        const size_t n = rand() % 9 + 1;
        for (size_t i = 0; i < n; ++i)
        {
            c.map.emplace(randomString(3, 8), randomSet(rand() % 4 + 1, 3, 8));
        }
        form[std::move(s)] = std::move(c);
    }
}

template <> inline void addForm<StringSelection>(Form &form, String &&s, StringSelection &&selection)
{
    selection.set = randomSet(rand() % 4 + 2, 3, 8);
    selection.index = rand() % selection.set.size();
    form[std::move(s)] = std::move(selection);
}

template <> inline void addForm<StringMap>(Form &form, String &&s, StringMap &&map)
{
    const size_t n = rand() % 10 + 2;
    for (size_t i = 0; i < n; ++i)
    {
        map.emplace(randomString(3, 8), rand() % 2 == 0);
    }
    form[std::move(s)] = std::move(map);
}
} // namespace CanForm