#pragma once

#include <map>
#include <memory>
#include <memory_resource>
#include <set>
#include <string>
#include <string_view>
#include <variant>

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

using Number = std::variant<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double>;

using String = std::pmr::string;
using StringSet = std::pmr::set<String>;
struct StringSelection
{
    StringSet set;
    int index;

    StringSelection() : set(), index(0)
    {
    }
    StringSelection(const StringSelection &) = default;
    StringSelection(StringSelection &&) noexcept = default;

    StringSelection &operator=(const StringSelection &) = default;
    StringSelection &operator=(StringSelection &&) noexcept = default;

    constexpr bool valid() const noexcept
    {
        return 0 <= index && index < static_cast<int>(set.size());
    }
};
using StringMap = std::pmr::map<String, bool>;

using FormData = std::variant<bool, Number, String, StringSelection, StringMap>;
using Form = std::pmr::map<String, FormData>;

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