#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <thread>
#include <ztrace/ztrace.hpp>

std::random_device rd;
std::mt19937 gen(rd());

struct Counter {
  alignas(64) std::atomic_uint64_t produced{0};
  alignas(64) std::atomic_uint64_t dropped{0};
};

constexpr size_t THREADS = 10;

std::vector<std::jthread> threads;
std::vector<std::unique_ptr<Counter>> counters;

void launchThread(size_t i, const std::string &varName) {
  counters.push_back(std::make_unique<Counter>());
  threads.emplace_back([&, storageName = varName, index = i]() {
    std::random_device localRd;
    std::mt19937 localGen(localRd());

    ztrace::Stream<int64_t> stream(storageName);

    int64_t minVal = std::uniform_int_distribution<int64_t>(10, 500)(localGen);
    int64_t maxVal = std::uniform_int_distribution<int64_t>(500, 1000000)(localGen);
    std::uniform_int_distribution<int64_t> dist(minVal, maxVal);

    size_t noDropCounter = 0;
    while (true) {
      int64_t value = dist(localGen);
      if (stream.push(value)) {
        counters[index]->produced.fetch_add(1);
        if (++noDropCounter > 1000) {
          counters[index]->dropped = 0;
        }
      } else {
        noDropCounter = 0;
        counters[index]->dropped.fetch_add(1);
      }
    }
  });
}

int main() {
  ztrace::init();
  threads.reserve(THREADS);
  counters.reserve(THREADS);

  for (size_t i = 0; i < THREADS; ++i) {
    const std::string name = "stream_" + std::to_string(i);
    launchThread(i, name);
  }

  std::cout << "All threads started. Press Ctrl+C to stop." << std::endl;

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Generator stats" << std::endl;
    int i = 0;
    for (auto &counter : counters) {
      std::cout << "G " << i << " p: " << counter->produced << ", d: " << counter->dropped
                << std::endl;
      ++i;
    }
  }

  return 0;
}