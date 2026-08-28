#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ztrace::detail {

namespace ipc = boost::interprocess;

class Shm {
public:
  Shm(std::string_view name, std::size_t size)
      : shm_(ipc::open_or_create, std::string(name).c_str(), ipc::read_write) {
    ipc::offset_t current_size = 0;

    if (!shm_.get_size(current_size)) {
      throw std::runtime_error("Failed to get shared memory size: " + std::string(name));
    }

    if (current_size == 0) {
      shm_.truncate(static_cast<ipc::offset_t>(size));
    } else if (current_size != static_cast<ipc::offset_t>(size)) {
      throw std::runtime_error("Shared memory size mismatch: expected " + std::to_string(size) +
                               ", actual " + std::to_string(current_size));
    }

    region_ = ipc::mapped_region(shm_, ipc::read_write);

    if (!region_.get_address()) {
      throw std::runtime_error("Failed to map shared memory: " + std::string(name));
    }
  }

  Shm(const Shm &) = delete;
  Shm &operator=(const Shm &) = delete;

  Shm(Shm &&) = delete;
  Shm &operator=(Shm &&) = delete;

  ~Shm() = default;

  void *data() noexcept { return region_.get_address(); }

  const void *data() const noexcept { return region_.get_address(); }

  std::size_t size() const noexcept { return region_.get_size(); }

private:
  ipc::shared_memory_object shm_;
  ipc::mapped_region region_;
};

} // namespace ztrace::detail