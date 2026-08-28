#pragma once

#include <atomic>

#include "ztrace/config.hpp"
#include "ztrace/types.hpp"

namespace ztrace::detail {

inline constexpr size_t MAX_NAME_LENGTH = 32;

template <typename T>
struct alignas(64) VariableStorage {
  char name[MAX_NAME_LENGTH];
  VarType type;
  MemoryOrder order;
  std::atomic<T> value;
};

static_assert(sizeof(VariableStorage<int64_t>) <= 64);
static_assert(sizeof(VariableStorage<double>) <= 64);

} // namespace ztrace::detail