#pragma once

#include <cstddef>

#include "detail/storage_header.hpp"
#include "ztrace/config.hpp"
#include "ztrace/detail/spsc_ring_buffer.hpp"
#include "ztrace/stream.hpp"
#include "ztrace/variable.hpp"
#include "ztrace/viewer.hpp"

namespace ztrace {
void init(size_t size = DEFAULT_SHM_SIZE) { detail::ShmManager::instance().init(size); }
auto variables() -> std::vector<detail::StorageHeader> {
  return detail::ShmManager::instance().variables();
}
auto streams() -> std::vector<detail::StorageHeader> {
  return detail::ShmManager::instance().streams();
}
} // namespace ztrace