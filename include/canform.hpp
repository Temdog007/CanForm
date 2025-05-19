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

struct Done
{
    virtual ~Done()
    {
    }
    virtual bool isDone() = 0;
};
extern void waitUntilMessage(std::string_view title, std::string_view message, Done &, void *parent = nullptr);

template <typename F, std::enable_if_t<std::is_invocable_r<bool, F>::value, bool> = true>
static inline void waitUntilMessage(std::string_view title, std::string_view message, F func, void *parent = nullptr)
{
    struct Func
    {
        F &func;
        Func(F &f) : func(f)
        {
        }
        virtual ~Func()
        {
        }
        bool isDone()
        {
            return func();
        }
    };
    Func done(func);
    waitUntilMessage(title, message, done, parent);
}

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

using FormData = std::variant<bool, long, String, StringSelection, StringMap>;
using Form = std::pmr::map<String, FormData>;

extern DialogResult executeForm(std::string_view, Form &, void *parent = nullptr);

} // namespace CanForm