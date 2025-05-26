#pragma once

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
extern bool askQuestion(std::string_view title, std::string_view question, void *parent = nullptr);

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

} // namespace CanForm