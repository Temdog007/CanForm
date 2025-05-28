#pragma once

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

struct QuestionResponse
{
    virtual ~QuestionResponse()
    {
    }
    virtual void yes() = 0;
    virtual void no()
    {
    }
};

template <typename F> class QuestionResponseLambda : public QuestionResponse
{
  private:
    F func;

  public:
    QuestionResponseLambda(F &&f) noexcept : func(std::move(f))
    {
    }
    virtual ~QuestionResponseLambda()
    {
    }

    virtual void yes() override
    {
        func();
    }
};

template <typename F> static inline QuestionResponseLambda<F> respondToQuestion(F &&f) noexcept
{
    return QuestionResponseLambda(std::move(f));
}

extern void askQuestion(std::string_view title, std::string_view question, const std::shared_ptr<QuestionResponse> &,
                        void *parent = nullptr);

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
        virtual void canceled()
        {
        }
    };
    void show(const std::shared_ptr<Handler> &, void *parent = nullptr) const;
};

} // namespace CanForm