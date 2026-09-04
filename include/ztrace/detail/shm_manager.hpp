#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
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
        hdr->total_size = size;
        hdr->used_size = sizeof(ShmHeader);
        hdr->variable_count = 0;
        hdr->stream_count = 0;
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

  std::vector<std::string> variables() const {
    std::vector<std::string> result;
    result.reserve(header()->variable_count);

    scan([&](const StorageHeader *storage) {
      if (storage->storage_type == StorageType::Variable) {
        result.emplace_back(storage->name);
      }
    });

    return result;
  }

  std::vector<std::string> streams() const {
    std::vector<std::string> result;
    result.reserve(header()->stream_count);

    scan([&](const StorageHeader *storage) {
      if (storage->storage_type == StorageType::Stream) {
        result.emplace_back(storage->name);
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

    size_t offset = sizeof(ShmHeader);

    while (offset < hdr->used_size) {
      auto *storage = reinterpret_cast<StorageHeader *>(base + offset);

      if (storage->storage_type == StorageType::Stream && std::string_view(storage->name) == name) {
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
    const size_t offset = hdr->used_size;

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

    hdr->used_size += storage_size;
    ++hdr->variable_count;

    return storage;
  }

  template <typename T, size_t Capacity>
  StreamStorage<T, Capacity> *create_stream(std::string_view name, int32_t update_rate) {

    auto *hdr = header();

    constexpr size_t storage_size = sizeof(StreamStorage<T, Capacity>);

    const size_t offset = hdr->used_size;

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

    hdr->used_size += storage_size;
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
  bool initialized_ = false;
};

} // namespace ztrace::detail