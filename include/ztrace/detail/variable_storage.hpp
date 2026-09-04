#pragma once

#include <atomic>

#include "storage_type.hpp"
#include "ztrace/config.hpp"
#include "ztrace/types.hpp"

namespace ztrace::detail {

template <typename T>
struct alignas(CACHE_LINE_SIZE) VariableStorage {
  StorageType storage_type;
  DataType data_type;
  MemoryOrder order;
  uint32_t update_rate;
  uint32_t data_size;
  alignas(VALUE_ALIGNMENT) std::atomic<T> value;
  alignas(NAME_ALIGNMENT) char name[MAX_NAME_LENGTH];
};

static_assert(offsetof(VariableStorage<int32_t>, storage_type) == 0);
static_assert(offsetof(VariableStorage<int64_t>, storage_type) == 0);
static_assert(offsetof(VariableStorage<float>, storage_type) == 0);
static_assert(offsetof(VariableStorage<double>, storage_type) == 0);

static_assert(offsetof(VariableStorage<int32_t>, data_type) == 1);
static_assert(offsetof(VariableStorage<int64_t>, data_type) == 1);
static_assert(offsetof(VariableStorage<float>, data_type) == 1);
static_assert(offsetof(VariableStorage<double>, data_type) == 1);

static_assert(offsetof(VariableStorage<int32_t>, order) == 2);
static_assert(offsetof(VariableStorage<int64_t>, order) == 2);
static_assert(offsetof(VariableStorage<float>, order) == 2);
static_assert(offsetof(VariableStorage<double>, order) == 2);

static_assert(offsetof(VariableStorage<int32_t>, update_rate) == 4);
static_assert(offsetof(VariableStorage<int64_t>, update_rate) == 4);
static_assert(offsetof(VariableStorage<float>, update_rate) == 4);
static_assert(offsetof(VariableStorage<double>, update_rate) == 4);

static_assert(offsetof(VariableStorage<int32_t>, data_size) == 8);
static_assert(offsetof(VariableStorage<int64_t>, data_size) == 8);
static_assert(offsetof(VariableStorage<float>, data_size) == 8);
static_assert(offsetof(VariableStorage<double>, data_size) == 8);

static_assert(offsetof(VariableStorage<int32_t>, value) == VALUE_ALIGNMENT);
static_assert(offsetof(VariableStorage<int64_t>, value) == VALUE_ALIGNMENT);
static_assert(offsetof(VariableStorage<float>, value) == VALUE_ALIGNMENT);
static_assert(offsetof(VariableStorage<double>, value) == VALUE_ALIGNMENT);

static_assert(offsetof(VariableStorage<int32_t>, name) == NAME_ALIGNMENT);
static_assert(offsetof(VariableStorage<int64_t>, name) == NAME_ALIGNMENT);
static_assert(offsetof(VariableStorage<float>, name) == NAME_ALIGNMENT);
static_assert(offsetof(VariableStorage<double>, name) == NAME_ALIGNMENT);

static_assert(sizeof(VariableStorage<int32_t>) == CACHE_LINE_SIZE);
static_assert(sizeof(VariableStorage<int64_t>) == CACHE_LINE_SIZE);

} // namespace ztrace::detail