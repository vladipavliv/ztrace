#pragma once

#include <atomic>

namespace ztrace::detail {

struct alignas(64) ShmHeader {
  uint64_t magic;
  uint32_t version;
  std::atomic<uint32_t> total_size;
  std::atomic<uint32_t> used_size;
  std::atomic<uint32_t> variable_count;
  std::atomic<uint32_t> stream_count;
};

static_assert(sizeof(ShmHeader) == 64);
static_assert(std::atomic<uint32_t>::is_always_lock_free);

} // namespace ztrace::detail