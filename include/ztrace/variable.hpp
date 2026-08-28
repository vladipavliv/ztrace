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
      : storage_(detail::ShmManager::instance().get_variable<T>(name, order)),
        value_(storage_.value),
        order_(storage_.order == MemoryOrder::AcquireRelease ? std::memory_order_release
                                                             : std::memory_order_relaxed) {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    update(initial);
  }

  ~Variable() = default;

  Variable(const Variable &) = delete;
  Variable &operator=(const Variable &) = delete;
  Variable(Variable &&) = delete;
  Variable &operator=(Variable &&) = delete;

  void update(T value) { value_.store(value, order_); }

private:
  detail::VariableStorage<T> &storage_;
  std::atomic<T> &value_;
  const std::memory_order order_;
};

} // namespace ztrace