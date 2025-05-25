#pragma once

#include "types.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace CanForm
{
enum class MessageBoxType
{
    Information,
    Warning,
    Error
};

extern void showMessageBox(MessageBoxType, std::string_view title, std::string_view message, void *parent = nullptr);

struct MenuItem
{
    String label;
    virtual ~MenuItem()
    {
    }

    virtual bool onClick() = 0;
};

using MenuItems = std::pmr::vector<std::unique_ptr<MenuItem>>;

template <typename F> class MenuItemLambda : public MenuItem
{
  private:
    F func;

  public:
    MenuItemLambda(F &&f) noexcept : func(std::move(f))
    {
    }
    virtual ~MenuItemLambda()
    {
    }

    virtual bool onClick() override
    {
        return func();
    }
};

template <typename F> MenuItemLambda<F> makeItemMenu(String &&s, F &&f)
{
    MenuItemLambda menuItem(std::move(f));
    menuItem.label = std::move(s);
    return menuItem;
}

struct Menu
{
    String title;
    MenuItems items;

    template <typename T, typename... Args> void add(Args &&...args)
    {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        items.emplace_back(std::move(ptr));
    }

    template <typename F> void add(String &&s, F &&f)
    {
        auto ptr = std::make_unique<MenuItemLambda<F>>(std::move(f));
        ptr->label = std::move(s);
        items.emplace_back(std::move(ptr));
    }
};

using Menus = std::pmr::vector<Menu>;

struct MenuList
{
    Menus menus;
    virtual ~MenuList()
    {
    }

    void show(std::string_view, void *parent = nullptr);
};

enum class DialogResult
{
    Ok,
    Cancel,
    Error
};

struct FileDialog
{
    std::string_view title;
    std::string_view message;
    std::string_view startDirectory;
    std::string_view filename;
    bool directories;
    bool saving;
    bool multiple;

    constexpr FileDialog() noexcept
        : title(), message(), startDirectory(), filename(), directories(false), saving(false), multiple(false)
    {
    }

    struct Handler
    {
        virtual ~Handler()
        {
        }
        virtual bool handle(std::string_view) = 0;
    };
    DialogResult show(Handler &, void *parent = nullptr) const;
};

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

    template <typename T, size_t N> StringSelection(const std::array<T, N> &array) : StringSelection()
    {
        for (auto item : array)
        {
            set.emplace(item);
        }
    }

    template <typename T> StringSelection(std::initializer_list<T> list) : StringSelection()
    {
        for (auto item : list)
        {
            set.emplace(item);
        }
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

struct FormData;
struct Form;

struct MultiForm
{
    std::pmr::map<String, Form> tabs;
    String selected;
};

struct FormData
{
    using Data = std::variant<bool, Number, String, ComplexString, StringSelection, StringMap, MultiForm>;
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

extern DialogResult executeForm(std::string_view, Form &, size_t columns, void *parent = nullptr);

struct AsyncForm
{
    Form form;

    virtual ~AsyncForm()
    {
    }

    virtual void onSubmit(DialogResult) = 0;
    static void show(const std::shared_ptr<AsyncForm> &, std::string_view, size_t columns, void *parent = nullptr);
};

template <typename F> struct AsyncFormLambda : public AsyncForm
{
    static_assert(std::is_invocable<F, DialogResult>::value || std::is_invocable<F, Form &, DialogResult>::value);
    F func;

    AsyncFormLambda(F &&f) noexcept : func(std::move(f))
    {
    }

    virtual ~AsyncFormLambda()
    {
    }

    virtual void onSubmit(DialogResult result) override
    {
        if constexpr (std::is_invocable<F, DialogResult>::value)
        {
            func(result);
        }
        else if constexpr (std::is_invocable<F, Form &, DialogResult>::value)
        {
            func(form, result);
        }
    }
};

template <typename F>
void showAsyncForm(Form &&form, std::string_view title, F &&f, size_t columns, void *parent = nullptr)
{
    std::shared_ptr<AsyncForm> asyncForm = std::make_shared<AsyncFormLambda<F>>(std::move(f));
    asyncForm->form = std::move(form);
    AsyncForm::show(asyncForm, title, columns, parent);
}

extern String randomString(size_t min, size_t max);
extern String randomString(size_t n);
} // namespace CanForm
