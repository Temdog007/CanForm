#pragma once

#include "types.hpp"

namespace CanForm
{
struct Awaiter
{
    virtual ~Awaiter()
    {
    }
    virtual bool isDone() = 0;
};

extern void showPopupUntil(std::string_view message, const std::shared_ptr<Awaiter> &, size_t checkRate,
                           void *parent = nullptr);

struct DefaultAwaiter : public Awaiter
{
  protected:
    bool done;

  public:
    DefaultAwaiter() : done(false)
    {
    }

    virtual ~DefaultAwaiter()
    {
    }

    void setDone()
    {
        done = true;
    }
    virtual bool isDone() override
    {
        return done;
    }
};

template <typename Rep, typename Period> class TimeAwaiter : public Awaiter
{
  private:
    TimePoint start;

    using D = std::chrono::duration<Rep, Period>;
    D duration;

  public:
    TimeAwaiter(const D &d) : start(now()), duration(d)
    {
    }

    virtual ~TimeAwaiter()
    {
    }

    constexpr TimePoint getStart() const
    {
        return start;
    }

    virtual bool isDone() override
    {
        return now() - start > duration;
    }
};

template <typename Rep, typename Period>
static inline void showPopupUntil(std::string_view message, const std::chrono::duration<Rep, Period> &duration,
                                  size_t checkRate, void *parent = nullptr)
{
    auto awaiter = std::make_shared<TimeAwaiter<Rep, Period>>(duration);
    showPopupUntil(message, awaiter, checkRate, parent);
}

} // namespace CanForm