#pragma once

#include <atomic>

#include "ztrace/config.hpp"
#include "ztrace/types.hpp"

namespace ztrace::detail {

inline constexpr size_t MAX_NAME_LENGTH = 32;

template <typename T>
struct alignas(64) VariableStorage {
  std::atomic<T> value;
  alignas(VALUE_ALIGNMENT) char name[MAX_NAME_LENGTH];
  uint32_t update_rate;
  VarType type;
  MemoryOrder order;
};

static_assert(offsetof(VariableStorage<int32_t>, name) == 8);
static_assert(offsetof(VariableStorage<int64_t>, name) == 8);
static_assert(offsetof(VariableStorage<float>, name) == 8);
static_assert(offsetof(VariableStorage<double>, name) == 8);

static_assert(offsetof(VariableStorage<int32_t>, update_rate) == 40);
static_assert(offsetof(VariableStorage<int64_t>, update_rate) == 40);

static_assert(sizeof(VariableStorage<int32_t>) == 64);
static_assert(sizeof(VariableStorage<int64_t>) == 64);

} // namespace ztrace::detail