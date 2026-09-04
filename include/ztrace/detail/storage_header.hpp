#pragma once
#include <cstdint>

#include "storage_type.hpp"
#include "stream_storage.hpp"
#include "variable_storage.hpp"
#include "ztrace/config.hpp"
#include "ztrace/types.hpp"

namespace ztrace::detail {
struct alignas(CACHE_LINE_SIZE) StorageHeader {
  StorageType storage_type;
  DataType data_type;
  MemoryOrder order;
  uint32_t update_rate;
  uint32_t data_size;

  alignas(NAME_ALIGNMENT) char name[MAX_NAME_LENGTH];
};

static_assert(offsetof(VariableStorage<int32_t>, storage_type) ==
              offsetof(StorageHeader, storage_type));
static_assert(offsetof(StreamStorage<int32_t>, storage_type) ==
              offsetof(StorageHeader, storage_type));

static_assert(offsetof(VariableStorage<int32_t>, data_type) == offsetof(StorageHeader, data_type));
static_assert(offsetof(StreamStorage<int32_t>, data_type) == offsetof(StorageHeader, data_type));

static_assert(offsetof(VariableStorage<int32_t>, order) == offsetof(StorageHeader, order));
static_assert(offsetof(StreamStorage<int32_t>, order) == offsetof(StorageHeader, order));

static_assert(offsetof(VariableStorage<int32_t>, update_rate) ==
              offsetof(StorageHeader, update_rate));
static_assert(offsetof(StreamStorage<int32_t>, update_rate) ==
              offsetof(StorageHeader, update_rate));

static_assert(offsetof(VariableStorage<int32_t>, data_size) == offsetof(StorageHeader, data_size));
static_assert(offsetof(StreamStorage<int32_t>, data_size) == offsetof(StorageHeader, data_size));

static_assert(offsetof(VariableStorage<int32_t>, name) == offsetof(StorageHeader, name));
static_assert(offsetof(StreamStorage<int32_t>, name) == offsetof(StorageHeader, name));

static_assert(sizeof(StorageHeader) == CACHE_LINE_SIZE);

} // namespace ztrace::detail