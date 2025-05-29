#pragma once

#include <chrono>
#include <map>
#include <memory_resource>
#include <optional>
#include <set>
#include <string>
#include <variant>

#include "range.hpp"

namespace CanForm
{
using Number = std::variant<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double>;

using RangedValue = std::variant<Range<int8_t>, Range<int16_t>, Range<int32_t>, Range<int64_t>, Range<uint8_t>,
                                 Range<uint16_t>, Range<uint32_t>, Range<uint64_t>, Range<float>, Range<double>>;

using String = std::pmr::string;
using StringSet = std::pmr::set<String>;

using StringMap = std::pmr::map<String, bool>;

using TimePoint = std::chrono::system_clock::time_point;
extern TimePoint now();

} // namespace CanForm