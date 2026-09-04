#pragma once

#include <cstdint>

namespace ztrace {

enum class InitState : uint32_t { Initializing, Ready };

enum class DataType : uint8_t { Int32, Int64, Float, Double, Bool, Custom };

enum class MemoryOrder : uint8_t {
  Relaxed,       // memory_order_relaxed
  AcquireRelease // memory_order_acquire/release
};

} // namespace ztrace