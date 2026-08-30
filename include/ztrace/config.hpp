#pragma once

#include <cstddef>
#include <cstdint>

namespace ztrace {

inline constexpr int VERSION_MAJOR = 0;
inline constexpr int VERSION_MINOR = 1;
inline constexpr int VERSION_PATCH = 0;

inline constexpr const char *VERSION_STRING = "0.1.0";

inline constexpr size_t DEFAULT_RING_BUFFER_CAPACITY = 1024;
inline constexpr size_t DEFAULT_SHM_SIZE = 2 * 1024 * 1024;

inline constexpr size_t VALUE_ALIGNMENT = 8;

} // namespace ztrace