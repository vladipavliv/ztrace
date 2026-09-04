#pragma once

#include <cstring>
#include <memory>
#include <string_view>

#include "shm.hpp"
#include "variable_storage.hpp"
#include "ztrace/config.hpp"
#include "ztrace/types.hpp"

namespace ztrace::detail {

using ShmPtr = std::unique_ptr<Shm>;

struct alignas(64) ShmHeader {
  uint64_t magic;
  uint32_t version;
  uint32_t total_size;
  uint32_t used_size;
  uint32_t variable_count;
};

static_assert(sizeof(ShmHeader) == 64);

class ShmManager {
public:
  static ShmManager &instance() {
    static ShmManager manager;
    return manager;
  }

  ~ShmManager() {}

  void init(size_t size = DEFAULT_SHM_SIZE) {
    if (initialized_) {
      return;
    }

    try {
      shm_ = std::make_unique<Shm>("ztrace_shm", size);

      if (!shm_->data()) {
        throw std::runtime_error("Failed to create shared memory");
      }

      auto *header = static_cast<ShmHeader *>(shm_->data());
      if (header->magic != 0x5A5452414345ULL) {
        header->magic = 0x5A5452414345ULL;
        header->version = 1;
        header->total_size = size;
        header->used_size = sizeof(ShmHeader);
        header->variable_count = 0;
      } else {
      }

      initialized_ = true;
    } catch (const std::exception &e) {
      throw std::runtime_error("Failed to initialize shared memory: " + std::string(e.what()));
    }
  }

  bool is_initialized() const { return initialized_; }

  void *data() { return shm_ ? shm_->data() : nullptr; }
  const void *data() const { return shm_ ? shm_->data() : nullptr; }

  ShmHeader *header() { return static_cast<ShmHeader *>(data()); }
  const ShmHeader *header() const { return static_cast<const ShmHeader *>(data()); }

  template <typename T>
  VariableStorage<T> &get_variable(std::string_view name, int32_t update_rate = 1,
                                   MemoryOrder order = MemoryOrder::Relaxed) {
    if (!initialized_) {
      init();
    }

    auto *storage = find_variable<T>(name);
    if (storage) {
      if (storage->type != detect_type<T>()) {
        throw std::runtime_error("Type mismatch for variable: " + std::string(name));
      }
      if (storage->order != order) {
        throw std::runtime_error("Memory order mismatch for variable: " + std::string(name));
      }
      return *storage;
    }

    return *create_variable<T>(name, update_rate, order);
  }

  auto variables() const -> std::vector<std::string> {
    std::vector<std::string> result;
    result.reserve(header()->variable_count);

    const auto *base = static_cast<const std::byte *>(data());
    size_t offset = sizeof(ShmHeader);

    while (offset < header()->used_size) {
      const auto *storage = reinterpret_cast<const VariableStorage<int32_t> *>(base + offset);

      result.emplace_back(storage->name);
      offset += VARIABLE_SIZE;
    }
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
  template <typename T>
  VariableStorage<T> *find_variable(std::string_view name) {
    auto *hdr = header();
    char *base = static_cast<char *>(data());
    size_t offset = sizeof(ShmHeader);

    while (offset < hdr->used_size) {
      auto *storage = reinterpret_cast<VariableStorage<T> *>(base + offset);
      if (std::string_view(storage->name) == name) {
        if (storage->type != detect_type<T>()) {
          return nullptr;
        }
        return storage;
      }
      constexpr size_t alignment = alignof(VariableStorage<T>);
      offset = (offset + sizeof(VariableStorage<T>) + alignment - 1) & ~(alignment - 1);
    }

    return nullptr;
  }

  template <typename T>
  VariableStorage<T> *create_variable(std::string_view name, int32_t update_rate,
                                      MemoryOrder order) {
    auto *hdr = header();

    constexpr size_t alignment = alignof(VariableStorage<T>);
    size_t offset = (hdr->used_size + alignment - 1) & ~(alignment - 1);

    size_t storage_size = sizeof(VariableStorage<T>);

    if (offset + storage_size > hdr->total_size) {
      throw std::runtime_error("Not enough shared memory");
    }

    char *base = static_cast<char *>(data());
    auto *storage = new (base + offset) VariableStorage<T>();

    storage->value.store(T{}, std::memory_order_relaxed);

    const size_t copy_len = std::min(name.size(), MAX_NAME_LENGTH - 1);
    std::memcpy(storage->name, name.data(), copy_len);
    storage->name[copy_len] = '\0';

    storage->update_rate = update_rate;
    storage->type = detect_type<T>();
    storage->order = order;

    hdr->used_size = offset + storage_size;
    hdr->variable_count++;

    return storage;
  }

  template <typename T>
  static VarType detect_type() {
    if constexpr (std::is_same_v<T, int32_t>)
      return VarType::Int32;
    if constexpr (std::is_same_v<T, int64_t>)
      return VarType::Int64;
    if constexpr (std::is_same_v<T, float>)
      return VarType::Float;
    if constexpr (std::is_same_v<T, double>)
      return VarType::Double;
    if constexpr (std::is_same_v<T, bool>)
      return VarType::Bool;
    return VarType::Int64;
  }

  ShmPtr shm_;
  bool initialized_ = false;
};

} // namespace ztrace::detail