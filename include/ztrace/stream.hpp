#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

#include "ztrace/config.hpp"
#include "ztrace/detail/shm_manager.hpp"
#include "ztrace/detail/spsc_ring_buffer.hpp"
#include "ztrace/detail/stream_storage.hpp"

namespace ztrace {

template <typename T, size_t Capacity = DEFAULT_RING_BUFFER_CAPACITY>
class Stream {
public:
  explicit Stream(std::string_view name)
      : storage_(&detail::ShmManager::instance().get_stream<T, Capacity>(name)) {}

  bool push(const T &value) noexcept { return storage_->ring.push(value); }
  bool pop(T &value) noexcept { return storage_->ring.pop(value); }
  size_t drain(T *data, size_t size) noexcept { return storage_->ring.drain(data, size); }
  size_t size() const noexcept { return storage_->ring.size(); }
  bool empty() const noexcept { return storage_->ring.empty(); }
  bool full() const noexcept { return storage_->ring.full(); }
  void clear() noexcept { storage_->ring.clear(); }

  static constexpr size_t capacity() noexcept {
    return detail::SpscRingBuffer<T, Capacity>::capacity();
  }

private:
  detail::StreamStorage<T, Capacity> *storage_{};
};

} // namespace ztrace