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

struct Form;

struct StructForm
{
    using Map = std::pmr::map<String, Form>;
    Map map;
    size_t columns;

    StructForm() : map(), columns(1)
    {
    }
    StructForm(const StructForm &) = default;
    StructForm(StructForm &&) noexcept = default;
    template <typename... Args> StructForm(size_t c, Args &&...args) : map(std::forward<Args>(args)...), columns(c)
    {
    }

    StructForm &operator=(const StructForm &) = default;
    StructForm &operator=(StructForm &&) noexcept = default;

    template <typename T> StructForm &operator=(const Map &t)
    {
        map = t;
        return *this;
    }
    template <typename T> StructForm &operator=(Map &&t) noexcept
    {
        map = std::move(t);
        return *this;
    }

    template <typename K> auto &operator[](const K &k)
    {
        return map[k];
    }
    template <typename K> auto &operator[](K &&k)
    {
        return map[std::move(k)];
    }

    Map &operator*() noexcept
    {
        return map;
    }
    const Map &operator*() const noexcept
    {
        return map;
    }

    Map *operator->() noexcept;
    const Map *operator->() const noexcept;

    static StructForm create(StructForm &&structForm) noexcept;
    template <typename... Args> static StructForm create(StructForm &&, Args &&...args);
    template <typename K, typename V, typename... Args>
    static StructForm create(StructForm &&, K &&, V &&, Args &&...args);
    template <typename... Args> static StructForm create(Args &&...args);
};

struct VariantForm
{
    std::pmr::map<String, Form> tabs;
    String selected;
};

struct Form
{
    using Data = std::variant<bool, RangedValue, String, ComplexString, SortableList, StringSelection, StringMap,
                              VariantForm, StructForm>;
    Data data;

    Form() : data(false)
    {
    }
    Form(const Form &) = default;
    Form(Form &&) noexcept = default;
    template <typename... Args> Form(std::in_place_t, Args &&...args) : data(std::forward<Args>(args)...)
    {
    }

    Form &operator=(const Form &) = default;
    Form &operator=(Form &&) noexcept = default;

    template <typename T> Form &operator=(const T &t)
    {
        data = t;
        return *this;
    }
    template <typename T> Form &operator=(T &&t) noexcept
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

class FormExecute
{
  protected:
    Form form;

  public:
    FormExecute() = default;
    FormExecute(const FormExecute &) = delete;
    FormExecute(FormExecute &&) noexcept = default;
    template <typename... Args> FormExecute(std::in_place_t, Args &&...args) : form(std::forward<Args>(args)...)
    {
    }
    virtual ~FormExecute()
    {
    }

    FormExecute &operator=(const FormExecute &) = delete;
    FormExecute &operator=(FormExecute &&) noexcept = default;

    virtual void ok() = 0;
    virtual void cancel()
    {
    }

    const Form &getForm() const noexcept
    {
        return form;
    }

    static void execute(std::string_view, const std::shared_ptr<FormExecute> &, void *parent = nullptr);

    template <typename T, std::enable_if_t<std::is_base_of<FormExecute, T>::value, bool> = true>
    static inline void execute(std::string_view title, T &&t, void *parent = nullptr)
    {
        std::shared_ptr<T> ptr = std::make_shared<T>(std::move(t));
        execute(title, ptr, parent);
    }
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
    FormExecuteLambda(const FormExecuteLambda &) = delete;
    FormExecuteLambda(FormExecuteLambda &&) noexcept = default;
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
        else if constexpr (std::is_invocable<F, Form &&>::value)
        {
            func(std::move(form));
        }
        else
        {
            func();
        }
    }
};

template <typename F, typename... Args> static inline FormExecuteLambda<F> executeForm(F &&f, Args &&...args)
{
    FormExecuteLambda lambda(std::move(f), std::forward<Args>(args)...);
    return lambda;
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

template <typename K, typename V, typename... Args>
inline StructForm StructForm::create(StructForm &&structForm, K &&key, V &&value, Args &&...args)
{
    structForm[std::move(key)] = std::move(value);
    return create(std::move(structForm), std::forward<Args>(args)...);
}

template <typename... Args> inline StructForm StructForm::create(Args &&...args)
{
    StructForm structForm;
    return create(std::move(structForm), std::forward<Args>(args)...);
}
} // namespace CanForm
