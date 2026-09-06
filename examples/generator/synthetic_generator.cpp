#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <thread>

#include <ztrace/ztrace.hpp>

namespace {

struct GeneratorState {
  static constexpr double rangeMin = 100.0;
  static constexpr double rangeMax = 10000.0;

  static constexpr double windowFraction = 0.30;
  static constexpr double windowStepFraction = 0.05;
  static constexpr double attractorStep = 0.05;

  static constexpr double attraction = 5.0;

  double windowMin = rangeMin;
  double windowMax = rangeMin + (rangeMax - rangeMin) * windowFraction;

  double attractor = 0.5;

  int windowDirection = 1;
  int attractorDirection = 1;
};

double sampleAttractorDistribution(double attractor, double attraction, std::mt19937_64 &rng,
                                   std::uniform_real_distribution<double> &uniform) {
  attraction = std::max(1.0, attraction);

  const double triangleProbability = (attraction - 1.0) / (attraction + 1.0);

  if (uniform(rng) > triangleProbability) {
    return uniform(rng);
  }

  const double leftLength = attractor;
  const double rightLength = 1.0 - attractor;

  const double leftProbability = leftLength / (leftLength + rightLength);

  if (uniform(rng) < leftProbability) {
    const double r = uniform(rng);

    return attractor - leftLength * std::sqrt(r);
  }

  const double r = uniform(rng);

  return attractor + rightLength * (1.0 - std::sqrt(r));
}

int64_t generateValue(double min, double max, double attractor, std::mt19937_64 &rng,
                      std::uniform_real_distribution<double> &uniform) {
  const double position =
      sampleAttractorDistribution(attractor, GeneratorState::attraction, rng, uniform);
  const double value = min + position * (max - min);
  return static_cast<int64_t>(value);
}

} // namespace

int main() {
  ztrace::init();
  ztrace::Stream<int64_t> rttStream("rtt");

  std::atomic<bool> stopFlag{false};
  std::atomic<bool> bootstrapDone{false};

  std::atomic<std::uint64_t> produced{0};

  GeneratorState state;

  std::cout << "Streaming producer started.\n"
            << "Range: [" << GeneratorState::rangeMin << "; " << GeneratorState::rangeMax << "]\n"
            << "Window: " << GeneratorState::windowFraction * 100.0 << "%\n"
            << "Attraction: " << GeneratorState::attraction << "\n"
            << "\n"
            << "Bootstrap is running...\n"
            << "Press ENTER to start normal streaming.\n"
            << std::endl;

  std::thread bootstrapController([&] {
    std::cin.get();

    bootstrapDone.store(true, std::memory_order_release);

    std::cout << "\nBootstrap finished. Starting normal stream.\n" << std::endl;
  });

  std::thread producer([&] {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_real_distribution<double> uniform(0.0, 1.0);

    while (!bootstrapDone.load(std::memory_order_acquire) &&
           !stopFlag.load(std::memory_order_relaxed)) {
      constexpr std::size_t batchSize = 1024;

      for (std::size_t i = 0; i < batchSize && !bootstrapDone.load(std::memory_order_acquire) &&
                              !stopFlag.load(std::memory_order_relaxed);
           ++i) {

        const double value = GeneratorState::rangeMin +
                             uniform(rng) * (GeneratorState::rangeMax - GeneratorState::rangeMin);

        if (rttStream.push(static_cast<int64_t>(value))) {
          produced.fetch_add(1, std::memory_order_relaxed);
        } else {
          std::this_thread::yield();
        }
      }
    }

    while (!stopFlag.load(std::memory_order_relaxed)) {
      const double min = state.windowMin;
      const double max = state.windowMax;
      const double attractor = state.attractor;

      constexpr std::size_t batchSize = 1024;

      for (std::size_t i = 0; i < batchSize && !stopFlag.load(std::memory_order_relaxed); ++i) {
        const int64_t value = generateValue(min, max, attractor, rng, uniform);

        if (rttStream.push(value)) {
          produced.fetch_add(1, std::memory_order_relaxed);
        } else {
          std::this_thread::yield();
        }
      }
    }
  });

  std::uint64_t previousProduced = 0;

  while (!stopFlag.load(std::memory_order_relaxed)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!bootstrapDone.load(std::memory_order_acquire)) {
      continue;
    }

    const double fullRange = GeneratorState::rangeMax - GeneratorState::rangeMin;
    const double windowSize = fullRange * GeneratorState::windowFraction;
    const double windowStep = fullRange * GeneratorState::windowStepFraction;

    state.windowMin += state.windowDirection * windowStep;

    const double maxWindowMin = GeneratorState::rangeMax - windowSize;

    if (state.windowMin >= maxWindowMin) {
      state.windowMin = maxWindowMin;
      state.windowDirection = -1;
    } else if (state.windowMin <= GeneratorState::rangeMin) {
      state.windowMin = GeneratorState::rangeMin;
      state.windowDirection = 1;
    }

    state.windowMax = state.windowMin + windowSize;
    state.attractor += state.attractorDirection * GeneratorState::attractorStep;

    if (state.attractor >= 1.0) {
      state.attractor = 1.0;
      state.attractorDirection = -1;
    } else if (state.attractor <= 0.0) {
      state.attractor = 0.0;
      state.attractorDirection = 1;
    }

    const std::uint64_t currentProduced = produced.load(std::memory_order_relaxed);
    const std::uint64_t delta = currentProduced - previousProduced;
    previousProduced = currentProduced;

    const std::uint64_t rps = delta * 10;

    std::cout << (bootstrapDone.load(std::memory_order_relaxed) ? "STREAM" : "BOOTSTRAP")
              << " | RPS: " << rps << " | total: " << currentProduced << " | window: ["
              << static_cast<int64_t>(state.windowMin) << "; "
              << static_cast<int64_t>(state.windowMax) << "]" << " | attractor: " << state.attractor
              << std::endl;
  }

  stopFlag.store(true, std::memory_order_relaxed);
  producer.join();

  if (bootstrapController.joinable()) {
    bootstrapController.detach();
  }

  return 0;
}
