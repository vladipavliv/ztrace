#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>

#include "ztrace/config.hpp"

namespace ztrace::detail {

template <typename T, size_t Capacity = DEFAULT_RING_BUFFER_CAPACITY>
class SpscRingBuffer {
  static_assert(Capacity > 1, "Capacity must be greater than 1");
  static_assert(std::is_trivially_copyable_v<T>,
                "T must be trivially copyable for use in shared memory");
  static_assert(std::atomic<size_t>::is_always_lock_free,
                "size_t atomic must be lock-free for use in shared memory");

public:
  SpscRingBuffer() noexcept = default;

  SpscRingBuffer(const SpscRingBuffer &) = delete;
  SpscRingBuffer &operator=(const SpscRingBuffer &) = delete;

  SpscRingBuffer(SpscRingBuffer &&) = delete;
  SpscRingBuffer &operator=(SpscRingBuffer &&) = delete;

  bool push(const T &value) noexcept {
    const size_t head = head_.load(std::memory_order_relaxed);
    const size_t next = increment(head);

    if (next == tail_.load(std::memory_order_acquire)) {
      return false;
    }

    data_[head] = value;
    head_.store(next, std::memory_order_release);

    return true;
  }

  bool pop(T &value) noexcept {
    const size_t tail = tail_.load(std::memory_order_relaxed);

    if (tail == head_.load(std::memory_order_acquire)) {
      return false;
    }

    value = data_[tail];
    tail_.store(increment(tail), std::memory_order_release);

    return true;
  }

  std::vector<T> drain() {
    std::vector<T> result;

    const size_t tail = tail_.load(std::memory_order_relaxed);
    const size_t head = head_.load(std::memory_order_acquire);

    if (tail == head) {
      return result;
    }

    const size_t count = head >= tail ? head - tail : Capacity - tail + head;

    result.reserve(count);

    size_t index = tail;

    for (size_t i = 0; i < count; ++i) {
      result.push_back(data_[index]);
      index = increment(index);
    }

    tail_.store(head, std::memory_order_release);
    return result;
  }

  void clear() noexcept {
    const size_t head = head_.load(std::memory_order_acquire);
    tail_.store(head, std::memory_order_release);
  }

  size_t size() const noexcept {
    const size_t head = head_.load(std::memory_order_acquire);
    const size_t tail = tail_.load(std::memory_order_acquire);

    if (head >= tail) {
      return head - tail;
    }

    return Capacity - tail + head;
  }

  bool empty() const noexcept {
    return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
  }

  bool full() const noexcept {
    const size_t head = head_.load(std::memory_order_acquire);
    const size_t tail = tail_.load(std::memory_order_acquire);

    return increment(head) == tail;
  }

  static constexpr size_t capacity() noexcept { return Capacity - 1; }
  static constexpr size_t storage_capacity() noexcept { return Capacity; }

private:
  static constexpr size_t increment(size_t index) noexcept {
    return index + 1 == Capacity ? 0 : index + 1;
  }

  alignas(64) std::atomic<size_t> head_{0};
  alignas(64) std::atomic<size_t> tail_{0};

  T data_[Capacity]{};
};

} // namespace ztrace::detail