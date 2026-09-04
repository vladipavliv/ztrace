#pragma once

#include <atomic>

#include "spsc_ring_buffer.hpp"
#include "storage_type.hpp"
#include "ztrace/config.hpp"
#include "ztrace/types.hpp"

namespace ztrace::detail {

template <typename T, size_t Capacity = DEFAULT_RING_BUFFER_CAPACITY>
struct alignas(CACHE_LINE_SIZE) StreamStorage {
  StorageType storage_type;
  DataType data_type;
  MemoryOrder order;
  uint32_t update_rate;
  uint32_t data_size;

  alignas(NAME_ALIGNMENT) char name[MAX_NAME_LENGTH];
  alignas(CACHE_LINE_SIZE) SpscRingBuffer<T, Capacity> ring;

  inline static constexpr size_t DATA_SIZE = sizeof(SpscRingBuffer<T, Capacity>);
};

static_assert(offsetof(StreamStorage<int32_t>, storage_type) == 0);
static_assert(offsetof(StreamStorage<int64_t>, storage_type) == 0);
static_assert(offsetof(StreamStorage<float>, storage_type) == 0);
static_assert(offsetof(StreamStorage<double>, storage_type) == 0);

static_assert(sizeof(StreamStorage<int32_t>) % CACHE_LINE_SIZE == 0);
static_assert(sizeof(StreamStorage<int64_t>) % CACHE_LINE_SIZE == 0);
static_assert(sizeof(StreamStorage<float>) % CACHE_LINE_SIZE == 0);
static_assert(sizeof(StreamStorage<double>) % CACHE_LINE_SIZE == 0);

} // namespace ztrace::detail