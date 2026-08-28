#pragma once

#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

#include "ztrace/config.hpp"
#include "ztrace/detail/shm_manager.hpp"
#include "ztrace/detail/spsc_ring_buffer.hpp"

namespace ztrace {

template <typename T, size_t Capacity = DEFAULT_RING_BUFFER_CAPACITY>
class Stream {
  struct alignas(64) StreamHeader {
    uint64_t magic{};
    uint32_t version{};
    uint32_t reserved{};
    size_t capacity{};
    size_t element_size{};
    size_t total_size{};
  };

  static constexpr uint64_t MAGIC = 0x5354524DULL;
  static constexpr uint32_t VERSION = 1;

public:
  explicit Stream(std::string_view name) {
    const std::string shm_name = std::string("zt_stream_") + std::string(name);

    constexpr size_t expected_size =
        sizeof(StreamHeader) + sizeof(detail::SpscRingBuffer<T, Capacity>);

    shm_ = std::make_unique<detail::Shm>(shm_name, expected_size);

    auto *header = static_cast<StreamHeader *>(shm_->data());

    if (header->magic == 0) {
      header->magic = MAGIC;
      header->version = VERSION;
      header->capacity = Capacity;
      header->element_size = sizeof(T);
      header->total_size = expected_size;

      auto *buffer_start = reinterpret_cast<char *>(header) + sizeof(StreamHeader);

      ring_ = new (buffer_start) detail::SpscRingBuffer<T, Capacity>();
    } else {
      if (header->magic != MAGIC) {
        throw std::runtime_error("Invalid stream magic");
      }

      if (header->version != VERSION) {
        throw std::runtime_error("Stream version mismatch");
      }

      if (header->capacity != Capacity) {
        throw std::runtime_error("Stream capacity mismatch");
      }

      if (header->element_size != sizeof(T)) {
        throw std::runtime_error("Stream element size mismatch");
      }

      if (header->total_size != expected_size) {
        throw std::runtime_error("Stream size mismatch");
      }

      auto *buffer_start = reinterpret_cast<char *>(header) + sizeof(StreamHeader);

      ring_ = reinterpret_cast<detail::SpscRingBuffer<T, Capacity> *>(buffer_start);
    }
  }

  bool push(const T &value) noexcept { return ring_->push(value); }
  bool pop(T &value) noexcept { return ring_->pop(value); }
  void drain(std::vector<T> &vec) { ring_->drain(vec); }

  size_t size() const noexcept { return ring_->size(); }
  bool empty() const noexcept { return ring_->empty(); }
  bool full() const noexcept { return ring_->full(); }
  void clear() noexcept { ring_->clear(); }

  static constexpr size_t capacity() noexcept {
    return detail::SpscRingBuffer<T, Capacity>::capacity();
  }

private:
  std::unique_ptr<detail::Shm> shm_;
  detail::SpscRingBuffer<T, Capacity> *ring_{};
};

} // namespace ztrace