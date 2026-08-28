#pragma once

#include <atomic>
#include <string_view>

#include "config.hpp"
#include "detail/shm_manager.hpp"
#include "types.hpp"

namespace ztrace {

template <typename T>
class Variable {
public:
  explicit Variable(std::string_view name, T initial = T{},
                    MemoryOrder order = MemoryOrder::Relaxed)
      : storage_(allocate_storage(name, order)), value_(storage_.value),
        order_(storage_.order == MemoryOrder::AcquireRelease ? std::memory_order_release
                                                             : std::memory_order_relaxed) {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    update(initial);
  }

  void update(T value) { value_.store(value, order_); }

private:
  static detail::VariableStorage<T> &allocate_storage(std::string_view name, MemoryOrder order) {
    auto &manager = detail::ShmManager::instance();
    return *(manager.get_variable<T>(name, order));
  }

  detail::VariableStorage<T> &storage_;
  std::atomic<T> &value_;
  const std::memory_order order_;
};

} // namespace ztrace