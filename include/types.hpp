#pragma once

#include <chrono>
#include <map>
#include <memory_resource>
#include <set>
#include <string>
#include <variant>

namespace CanForm
{
using Number = std::variant<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double>;

using String = std::pmr::string;
using StringSet = std::pmr::set<String>;

using StringMap = std::pmr::map<String, bool>;

using StringList = std::pmr::vector<std::pair<String, void *>>;

using TimePoint = std::chrono::system_clock::time_point;
} // namespace CanForm