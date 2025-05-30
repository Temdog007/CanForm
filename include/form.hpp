#pragma once

#include "dialog.hpp"
#include "types.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace CanForm
{
struct ComplexString
{
    std::pmr::map<String, StringSet> map;
    String string;
};

struct StringSelection
{
    StringSet set;
    int index;

    StringSelection() : set(), index(0)
    {
    }
    StringSelection(const StringSelection &) = default;
    StringSelection(StringSelection &&) noexcept = default;

    template <typename... Args> StringSelection(int i, Args &&...args) : set(std::forward<Args>(args)...), index(i)
    {
    }

    auto getIterator() const noexcept
    {
        int i = index;
        for (auto iter = set.begin(); iter != set.end(); ++iter, --i)
        {
            if (i == 0)
            {
                return iter;
            }
        }
        return set.end();
    }

    std::optional<String> getSelection() const
    {
        auto iter = getIterator();
        if (iter == set.end())
        {
            return std::nullopt;
        }
        return *iter;
    }

    bool setSelection(std::string_view newSelection)
    {
        int i = 0;
        for (auto &s : set)
        {
            if (s == newSelection)
            {
                index = i;
                return true;
            }
            ++i;
        }
        return false;
    }

    StringSelection &operator=(const StringSelection &) = default;
    StringSelection &operator=(StringSelection &&) noexcept = default;

    constexpr bool valid() const noexcept
    {
        return 0 <= index && index < static_cast<int>(set.size());
    }
};

static inline StringMap createStringMap(StringMap &map) noexcept
{
    return std::move(map);
}

template <typename T, typename... Args> StringMap createStringMap(StringMap &map, const T &t, Args &&...args)
{
    map[t] = false;
    return createStringMap(map, std::forward<Args>(args)...);
}

template <typename... Args> StringMap createStringMap(Args &&...args)
{
    StringMap map;
    return createStringMap(map, std::forward<Args>(args)...);
}

struct SortableItem
{
    String name;
    void *data;

    SortableItem() : name(), data(nullptr)
    {
    }
};

using SortableList = std::pmr::vector<SortableItem>;

struct FormData;
struct Form;

struct MultiForm
{
    std::pmr::map<String, Form> tabs;
    String selected;
};

struct FormData
{
    using Data =
        std::variant<bool, RangedValue, String, ComplexString, SortableList, StringSelection, StringMap, MultiForm>;
    Data data;

    FormData() : data(false)
    {
    }
    FormData(const FormData &) = default;
    FormData(FormData &&) noexcept = default;
    template <typename... Args> FormData(Args &...args) : data(std::forward<Args>(args)...)
    {
    }

    FormData &operator=(const FormData &) = default;
    FormData &operator=(FormData &&) noexcept = default;

    template <typename T> FormData &operator=(const T &t)
    {
        data = t;
        return *this;
    }
    template <typename T> FormData &operator=(T &&t) noexcept
    {
        data = std::move(t);
        return *this;
    }

    Data &operator*() noexcept
    {
        return data;
    }
    const Data &operator*() const noexcept
    {
        return data;
    }

    Data *operator->() noexcept
    {
        return &data;
    }
    const Data *operator->() const noexcept
    {
        return &data;
    }
};

struct Form
{
    using Datas = std::pmr::map<String, FormData>;
    Datas datas;

    FormData &operator[](const String &key)
    {
        return datas.operator[](key);
    }
    FormData &operator[](String &&key)
    {
        return datas.operator[](std::move(key));
    }

    Datas &operator*() noexcept
    {
        return datas;
    }
    const Datas &operator*() const noexcept
    {
        return datas;
    }

    Datas *operator->() noexcept
    {
        return &datas;
    }
    const Datas *operator->() const noexcept
    {
        return &datas;
    }

    static Form create(Form &form) noexcept
    {
        return std::move(form);
    }

    template <typename... Args> static Form create(Args &&...args)
    {
        Form form;
        return create(form, std::forward<Args>(args)...);
    }

    template <typename K, typename V, typename... Args>
    static Form create(Form &form, const K &key, const V &t, Args &&...args)
    {
        form[key] = t;
        return create(form, std::forward<Args>(args)...);
    }

    template <typename K, typename V, typename... Args>
    static Form create(Form &form, const K &key, V &&t, Args &&...args)
    {
        form[key] = std::move(t);
        return create(form, std::forward<Args>(args)...);
    }

    template <typename K, typename V, typename... Args>
    static Form create(Form &form, K &&key, const V &t, Args &&...args)
    {
        form[std::move(key)] = t;
        return create(form, std::forward<Args>(args)...);
    }

    template <typename K, typename V, typename... Args> static Form create(Form &form, K &&key, V &&t, Args &&...args)
    {
        form[std::move(key)] = std::move(t);
        return create(form, std::forward<Args>(args)...);
    }
};

class FormExecute
{
  protected:
    Form form;

  public:
    FormExecute() = default;
    FormExecute(const FormExecute &) = default;
    FormExecute(FormExecute &&) noexcept = default;
    template <typename... Args> FormExecute(std::in_place_t, Args &&...args) : form(std::forward<Args>(args)...)
    {
    }
    virtual ~FormExecute()
    {
    }

    FormExecute &operator=(const FormExecute &) = default;
    FormExecute &operator=(FormExecute &&) noexcept = default;

    virtual void ok() = 0;
    virtual void cancel()
    {
    }

    const Form &getForm() const noexcept
    {
        return form;
    }

    static void execute(std::string_view, const std::shared_ptr<FormExecute> &, size_t columns, void *parent = nullptr);
};

template <typename F> class FormExecuteLambda : public FormExecute
{
  private:
    F func;

  public:
    template <typename... Args>
    FormExecuteLambda(F &&f, Args &&...args)
        : FormExecute(std::in_place, std::forward<Args>(args)...), func(std::move(f))
    {
    }
    virtual ~FormExecuteLambda()
    {
    }

    virtual void ok() override
    {
        if constexpr (std::is_invocable<F, const Form &>::value)
        {
            func(form);
        }
        else if constexpr (std::is_invocable<F, Form &>::value)
        {
            func(form);
        }
        else
        {
            func();
        }
    }
};

template <typename F, typename... Args> static inline FormExecuteLambda<F> executeForm(F &&f, Args &&...args)
{
    return FormExecuteLambda(std::move(f), std::forward<Args>(args)...);
}

extern char randomCharacter();

extern String randomString(size_t min, size_t max);
extern String randomString(size_t n);

template <typename T> inline void randomString(T &s, size_t n)
{
    const size_t oldSize = s.size();
    while (s.size() - oldSize < n)
    {
        s.push_back(randomCharacter());
    }
}

template <typename T> inline void randomString(T &string, size_t min, size_t max)
{
    randomString(string, rand() % (max - min) + min);
}
} // namespace CanForm
