#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "shm.hpp"
#include "storage_header.hpp"
#include "stream_storage.hpp"
#include "variable_storage.hpp"
#include "ztrace/config.hpp"
#include "ztrace/types.hpp"

namespace ztrace::detail {

class ShmManager {
public:
  static ShmManager &instance() {
    static ShmManager manager;
    return manager;
  }

  ~ShmManager() = default;

  void init(size_t size = DEFAULT_SHM_SIZE) {
    std::scoped_lock<std::recursive_mutex> lock{mutex_};
    if (initialized_) {
      return;
    }

    try {
      shm_ = std::make_unique<Shm>("ztrace_shm", size);

      if (!shm_->data()) {
        throw std::runtime_error("Failed to create shared memory");
      }

      auto *hdr = header();

      if (hdr->magic != MAGIC) {
        hdr->magic = MAGIC;
        hdr->version = 1;
        hdr->total_size.store(size, std::memory_order_relaxed);
        hdr->used_size.store(sizeof(ShmHeader), std::memory_order_relaxed);
        hdr->variable_count.store(0, std::memory_order_relaxed);
        hdr->stream_count.store(0, std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_release);
      }
      initialized_ = true;
    } catch (const std::exception &e) {
      throw std::runtime_error("Failed to initialize shared memory: " + std::string(e.what()));
    }
  }

  bool is_initialized() const noexcept { return initialized_; }

  void *data() noexcept { return shm_ ? shm_->data() : nullptr; }
  const void *data() const noexcept { return shm_ ? shm_->data() : nullptr; }

  ShmHeader *header() noexcept { return static_cast<ShmHeader *>(data()); }
  const ShmHeader *header() const noexcept { return static_cast<const ShmHeader *>(data()); }

  template <typename T>
  VariableStorage<T> &get_variable(std::string_view name, int32_t update_rate = 1,
                                   MemoryOrder order = MemoryOrder::Relaxed) {
    if (!initialized_) {
      init();
    }
    if (auto *storage = find_variable<T>(name)) {
      if (storage->data_type != detect_type<T>()) {
        throw std::runtime_error("Type mismatch for variable: " + std::string(name));
      }
      if (storage->data_size != sizeof(T)) {
        throw std::runtime_error("Data size mismatch for variable: " + std::string(name));
      }
      if (storage->order != order) {
        throw std::runtime_error("Memory order mismatch for variable: " + std::string(name));
      }
      return *storage;
    }
    return *create_variable<T>(name, update_rate, order);
  }

  template <typename T, size_t Capacity = DEFAULT_RING_BUFFER_CAPACITY>
  StreamStorage<T, Capacity> &get_stream(std::string_view name, int32_t update_rate = 1) {
    if (!initialized_) {
      init();
    }
    if (auto *storage = find_stream<T, Capacity>(name)) {
      if (storage->data_type != detect_type<T>()) {
        throw std::runtime_error("Type mismatch for stream: " + std::string(name));
      }
      if (storage->data_size != sizeof(SpscRingBuffer<T, Capacity>)) {
        throw std::runtime_error("Ring buffer size mismatch for stream: " + std::string(name));
      }
      return *storage;
    }
    return *create_stream<T, Capacity>(name, update_rate);
  }

  auto variables() const -> std::vector<StorageHeader> {
    if (!initialized_) {
      throw std::runtime_error("ztrace not initialized");
    }
    std::vector<StorageHeader> result;
    result.reserve(header()->variable_count);

    scan([&](const StorageHeader *storage) {
      if (storage->storage_type == StorageType::Variable) {
        StorageHeader copy{storage->storage_type, storage->data_type, storage->order,
                           storage->update_rate,  storage->data_size, {0}};
        std::memcpy(copy.name, storage->name, MAX_NAME_LENGTH);
        result.emplace_back(copy);
      }
    });

    return result;
  }

  auto streams() const -> std::vector<StorageHeader> {
    if (!initialized_) {
      throw std::runtime_error("ztrace not initialized");
    }
    std::vector<StorageHeader> result;
    result.reserve(header()->stream_count);

    scan([&](const StorageHeader *storage) {
      if (storage->storage_type == StorageType::Stream) {
        StorageHeader copy{storage->storage_type, storage->data_type, storage->order,
                           storage->update_rate,  storage->data_size, {0}};
        std::memcpy(copy.name, storage->name, MAX_NAME_LENGTH);
        result.emplace_back(copy);
      }
    });

    return result;
  }

  void release() {
    if (!initialized_) {
      return;
    }

    shm_.reset();
    initialized_ = false;
  }

private:
  static constexpr uint64_t MAGIC = 0x5A5452414345ULL;

  template <typename Callback>
  void scan(Callback &&callback) const {
    const auto *hdr = header();
    const auto *base = static_cast<const std::byte *>(data());

    size_t offset = sizeof(ShmHeader);

    while (offset < hdr->used_size) {
      const auto *storage = reinterpret_cast<const StorageHeader *>(base + offset);
      callback(storage);
      offset += storage_size(storage);
    }
  }

  static size_t storage_size(const StorageHeader *storage) {
    switch (storage->storage_type) {
    case StorageType::Variable:
      return CACHE_LINE_SIZE;

    case StorageType::Stream:
      return CACHE_LINE_SIZE + storage->data_size;
    }

    throw std::runtime_error("Invalid storage type");
  }

  template <typename T>
  VariableStorage<T> *find_variable(std::string_view name) {
    const auto *hdr = header();
    auto *base = static_cast<std::byte *>(data());

    size_t offset = sizeof(ShmHeader);

    while (offset < hdr->used_size) {
      auto *storage = reinterpret_cast<StorageHeader *>(base + offset);

      if (storage->storage_type == StorageType::Variable &&
          std::string_view(storage->name) == name) {
        return reinterpret_cast<VariableStorage<T> *>(storage);
      }

      offset += storage_size(storage);
    }

    return nullptr;
  }

  template <typename T, size_t Capacity>
  StreamStorage<T, Capacity> *find_stream(std::string_view name) {
    const auto *hdr = header();
    auto *base = static_cast<std::byte *>(data());

    uint32_t used_size = hdr->used_size.load(std::memory_order_acquire);
    size_t offset = sizeof(ShmHeader);

    while (offset < used_size) {
      auto *storage = reinterpret_cast<StorageHeader *>(base + offset);

      if (storage->storage_type == StorageType::Stream && std::string_view(storage->name) == name) {
        std::atomic_thread_fence(std::memory_order_acquire);
        return reinterpret_cast<StreamStorage<T, Capacity> *>(storage);
      }

      offset += storage_size(storage);
    }

    return nullptr;
  }

  template <typename T>
  VariableStorage<T> *create_variable(std::string_view name, int32_t update_rate,
                                      MemoryOrder order) {
    auto *hdr = header();
    constexpr size_t storage_size = sizeof(VariableStorage<T>);

    uint32_t expectedOffset = hdr->used_size.load(std::memory_order_relaxed);
    while (!hdr->used_size.compare_exchange_weak(expectedOffset, expectedOffset + storage_size,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_relaxed)) {
      if (expectedOffset + storage_size > hdr->total_size) {
        throw std::runtime_error("Not enough shared memory");
      }
    }

    const uint32_t offset = expectedOffset;
    if (offset + storage_size > hdr->total_size) {
      throw std::runtime_error("Not enough shared memory");
    }

    auto *base = static_cast<std::byte *>(data());
    auto *storage = new (base + offset) VariableStorage<T>();

    storage->storage_type = StorageType::Variable;
    storage->data_type = detect_type<T>();
    storage->order = order;
    storage->update_rate = update_rate;
    storage->data_size = sizeof(T);

    storage->value.store(T{}, std::memory_order_relaxed);

    const size_t copy_len = std::min(name.size(), MAX_NAME_LENGTH - 1);

    std::memcpy(storage->name, name.data(), copy_len);
    storage->name[copy_len] = '\0';

    std::atomic_thread_fence(std::memory_order_release);
    ++hdr->variable_count;

    return storage;
  }

  template <typename T, size_t Capacity>
  StreamStorage<T, Capacity> *create_stream(std::string_view name, int32_t update_rate) {
    auto *hdr = header();

    constexpr size_t storage_size = sizeof(StreamStorage<T, Capacity>);

    uint32_t expectedOffset = hdr->used_size.load(std::memory_order_relaxed);
    while (!hdr->used_size.compare_exchange_weak(expectedOffset, expectedOffset + storage_size,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_relaxed)) {
      if (expectedOffset + storage_size > hdr->total_size) {
        throw std::runtime_error("Not enough shared memory");
      }
    }

    const uint32_t offset = expectedOffset;

    if (offset + storage_size > hdr->total_size) {
      throw std::runtime_error("Not enough shared memory");
    }

    auto *base = static_cast<std::byte *>(data());
    auto *storage = new (base + offset) StreamStorage<T, Capacity>();

    storage->storage_type = StorageType::Stream;
    storage->data_type = detect_type<T>();
    storage->order = MemoryOrder::Relaxed;
    storage->update_rate = update_rate;
    storage->data_size = sizeof(SpscRingBuffer<T, Capacity>);

    const size_t copy_len = std::min(name.size(), MAX_NAME_LENGTH - 1);

    std::memcpy(storage->name, name.data(), copy_len);
    storage->name[copy_len] = '\0';

    std::atomic_thread_fence(std::memory_order_release);
    ++hdr->stream_count;

    return storage;
  }

  template <typename T>
  static DataType detect_type() {
    if constexpr (std::is_same_v<T, int32_t>)
      return DataType::Int32;

    if constexpr (std::is_same_v<T, int64_t>)
      return DataType::Int64;

    if constexpr (std::is_same_v<T, float>)
      return DataType::Float;

    if constexpr (std::is_same_v<T, double>)
      return DataType::Double;

    if constexpr (std::is_same_v<T, bool>)
      return DataType::Bool;

    return DataType::Custom;
  }

  ShmPtr shm_;
  std::atomic_bool initialized_ = false;
  std::recursive_mutex mutex_;
};

} // namespace ztrace::detail
