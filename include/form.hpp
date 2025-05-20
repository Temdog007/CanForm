#pragma once

#include "types.hpp"

#include <memory>
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

struct RunAfter
{
    virtual ~RunAfter()
    {
    }
    virtual bool isReady() = 0;
    virtual void run()
    {
    }
};

template <typename Base, typename F> class RunAfterLambda : public Base
{
  private:
    static_assert(std::is_base_of<RunAfter, Base>::value);

    F func;

  public:
    template <typename... Args>
    RunAfterLambda(F &&f, Args &&...args) noexcept : Base(std::forward<Args>(args)...), func(std::move(f))
    {
    }
    virtual ~RunAfterLambda()
    {
    }

    virtual void run() override
    {
        func();
    }
};

template <typename Base, typename F, typename... Args> RunAfterLambda<Base, F> makeRunAfter(F &&func, Args &&...args)
{
    return RunAfterLambda<Base, F>(std::move(func), std::forward<Args>(args)...);
}

template <typename Base, typename F, typename... Args>
std::shared_ptr<RunAfterLambda<Base, F>> shareRunAfter(F &&func, Args &&...args)
{
    return std::make_shared<RunAfterLambda<Base, F>>(std::move(func), std::forward<Args>(args)...);
}

extern void waitUntilMessage(std::string_view title, std::string_view message, const std::shared_ptr<RunAfter> &,
                             void *parent = nullptr);

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
    std::string_view filters;
    bool directories;
    bool saving;
    bool multiple;

    constexpr FileDialog() noexcept
        : title(), message(), startDirectory(), filename(), filters(), directories(false), saving(false),
          multiple(false)
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
    using Data = std::variant<bool, Number, String, StringSelection, StringMap, MultiForm>;
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

extern DialogResult executeForm(std::string_view, Form &, void *parent = nullptr);

struct AsyncForm
{
    Form form;

    virtual ~AsyncForm()
    {
    }

    virtual void onSubmit(DialogResult) = 0;
    static void show(const std::shared_ptr<AsyncForm> &, std::string_view, void *parent = nullptr);
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

template <typename F> void showAsyncForm(Form &&form, std::string_view title, F &&f, void *parent = nullptr)
{
    std::shared_ptr<AsyncForm> asyncForm = std::make_shared<AsyncFormLambda<F>>(std::move(f));
    asyncForm->form = std::move(form);
    AsyncForm::show(asyncForm, title, parent);
}

} // namespace CanForm