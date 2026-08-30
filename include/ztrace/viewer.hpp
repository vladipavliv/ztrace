#pragma once

#include <atomic>
#include <string_view>
#include <type_traits>

#include "detail/shm_manager.hpp"
#include "types.hpp"

namespace ztrace {

template <typename T>
class Viewer {
public:
  explicit Viewer(std::string_view name)
      : storage_{detail::ShmManager::instance().get_variable<T>(name)}, value_(storage_.value),
        order_(storage_.order == MemoryOrder::AcquireRelease ? std::memory_order_acquire
                                                             : std::memory_order_relaxed) {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
  }

  ~Viewer() = default;

  Viewer(const Viewer &) = delete;
  Viewer &operator=(const Viewer &) = delete;
  Viewer(Viewer &&) = delete;
  Viewer &operator=(Viewer &&) = delete;

  T read() const { return value_.load(order_); }
  uint32_t update_rate() const { return storage_.update_rate; }

private:
  const detail::VariableStorage<T> &storage_;
  const std::atomic<T> &value_;
  const std::memory_order order_;
};

} // namespace ztrace