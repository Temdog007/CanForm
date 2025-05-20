#pragma once

#include <tuple>

#define TIE(T)                                                                                                         \
    bool operator==(const T &t) const noexcept                                                                         \
    {                                                                                                                  \
        return makeTie() == t.makeTie();                                                                               \
    }                                                                                                                  \
    bool operator!=(const T &t) const noexcept                                                                         \
    {                                                                                                                  \
        return makeTie() != t.makeTie();                                                                               \
    }                                                                                                                  \
    bool operator<=(const T &t) const noexcept                                                                         \
    {                                                                                                                  \
        return makeTie() <= t.makeTie();                                                                               \
    }                                                                                                                  \
    bool operator<(const T &t) const noexcept                                                                          \
    {                                                                                                                  \
        return makeTie() < t.makeTie();                                                                                \
    }                                                                                                                  \
    bool operator>=(const T &t) const noexcept                                                                         \
    {                                                                                                                  \
        return makeTie() >= t.makeTie();                                                                               \
    }                                                                                                                  \
    bool operator>(const T &t) const noexcept                                                                          \
    {                                                                                                                  \
        return makeTie() > t.makeTie();                                                                                \
    }
