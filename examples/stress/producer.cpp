#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <ztrace/ztrace.hpp>

#include "ztrace/detail/shm_manager.hpp"

using namespace std::chrono_literals;

inline std::string format_value(uint64_t value) {
  if (value >= 1'000'000'000)
    return std::to_string(value / 1'000'000'000) + "b";

  if (value >= 1'000'000)
    return std::to_string(value / 1'000'000) + "m";

  if (value >= 1'000)
    return std::to_string(value / 1'000) + "k";

  return std::to_string(value);
}

int main() {
  try {
    ztrace::init();

    std::cout << "Stress producer started. Updating value like there is no tomorrow" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    const std::string stressVarName = "stressVar";
    const std::string chillVarName = "chillVar";
    ztrace::Variable<uint64_t> stressVar(stressVarName, 0, 0, ztrace::MemoryOrder::Relaxed);
    ztrace::Variable<bool> chillVar(chillVarName, 1, 0, ztrace::MemoryOrder::Relaxed);

    const size_t doubleVarsCount = 10;
    std::vector<std::unique_ptr<ztrace::Variable<double>>> doubleVars;
    doubleVars.reserve(doubleVarsCount);
    for (size_t i = 0; i < doubleVarsCount; ++i) {
      const std::string name = "double" + std::to_string(i);
      auto varPtr = std::make_unique<ztrace::Variable<double>>(name, i + 1, 0);
      doubleVars.push_back(std::move(varPtr));
    }

    auto &var = ztrace::detail::ShmManager::instance().get_variable<uint64_t>(stressVarName);
    uint64_t lastValue = 0;
    bool chillValue = true;

    std::thread stressProducer([&] {
      uint64_t value = 0;
      while (true) {
        stressVar.update(++value);
      }
    });

    std::thread doubleProducer([&] {
      size_t tick = 0;
      std::vector<size_t> doubleCounters(doubleVarsCount);

      while (true) {
        for (size_t i = 0; i < doubleVars.size(); ++i) {
          const size_t interval = doubleVarsCount - i;

          if (tick % interval == 0) {
            doubleVars[i]->update(static_cast<double>(++doubleCounters[i]));
          }
        }

        tick = (tick + 1) % doubleVarsCount;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / doubleVarsCount));
      }
    });

    while (true) {
      std::this_thread::sleep_for(1s);

      chillValue = !chillValue;
      chillVar.update(chillValue);

      const uint64_t current = var.value.load(std::memory_order_relaxed);
      const uint64_t updates = current - lastValue;

      std::cout << "Updates/sec: " << format_value(updates) << std::endl;

      lastValue = current;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}