#ifndef _GENERIC_DEFINES_HPP
#define _GENERIC_DEFINES_HPP

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

using string = std::string;

template <typename ...Args>
using fstring = std::format_string<Args...>;

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8 = uint8_t;

using float64 = double;
using float32 = float;

using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8 = int8_t;

template <typename T>
using uptr = std::unique_ptr<T>;

template <typename ...Args>
using umap = std::unordered_map<Args...>;

template <typename ...Args>
using uset = std::unordered_set<Args...>;

#endif // _GENERIC_DEFINES_HPP
