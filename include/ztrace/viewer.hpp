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
      : storage_{find_storage(name)}, value_(storage_.value),
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

private:
  static const detail::VariableStorage<T> &find_storage(std::string_view name) {
    auto &manager = detail::ShmManager::instance();
    auto *storage = manager.get_variable<T>(name);
    if (!storage) {
      throw std::runtime_error("Failed to get variable: " + std::string(name));
    }
    return *storage;
  }

  const detail::VariableStorage<T> &storage_;
  const std::atomic<T> &value_;
  const std::memory_order order_;
};

} // namespace ztrace