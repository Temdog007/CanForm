#pragma once

#include "../form.hpp"

namespace CanForm
{
extern Form makeForm(bool makeInner = true);
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

template <typename T> inline void addForm(StructForm &structForm, String &&s, T &&t)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        t = random<T>();
        structForm[std::move(s)] = Range<T>(t);
    }
    else
    {
    }
}

template <> inline void addForm<bool>(StructForm &structForm, String &&s, bool &&b)
{
    b = rand() % 2 == 0;
    structForm[std::move(s)] = std::move(b);
}

template <> inline void addForm<String>(StructForm &structForm, String &&s, String &&value)
{
    value = randomString(3, 8);
    if (rand() % 2 == 0)
    {
        structForm[std::move(s)] = std::move(value);
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
        structForm[std::move(s)] = std::move(c);
    }
}

template <> inline void addForm<StringSelection>(StructForm &structForm, String &&s, StringSelection &&selection)
{
    selection.set = randomSet(rand() % 4 + 2, 3, 8);
    selection.index = rand() % selection.set.size();
    structForm[std::move(s)] = std::move(selection);
}

template <> inline void addForm<StringMap>(StructForm &structForm, String &&s, StringMap &&map)
{
    const size_t n = rand() % 10 + 2;
    for (size_t i = 0; i < n; ++i)
    {
        map.emplace(randomString(3, 8), rand() % 2 == 0);
    }
    structForm[std::move(s)] = std::move(map);
}

struct SimpleResponse : public QuestionResponse
{
    void *parent;
    constexpr SimpleResponse(void *p = nullptr) : parent(p)
    {
    }
    virtual ~SimpleResponse()
    {
    }

    virtual void yes() override
    {
        showMessageBox(MessageBoxType::Information, "Yes", "You selected yes", parent);
    }
    virtual void no() override
    {
        showMessageBox(MessageBoxType::Information, "No", "You selected no", parent);
    }
};
} // namespace CanForm