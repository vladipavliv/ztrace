#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <ztrace/ztrace.hpp>

struct FrameData {
  float x;
  float y;
  float z;
  uint64_t timestamp;
};

int main() {
  ztrace::init();

  ztrace::Stream<FrameData> frame_stream("frames");

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> pos_dist(-10.0, 10.0);

  std::cout << "Streaming producer started. Generating frame data..." << std::endl;

  uint64_t timestamp = 0;
  while (true) {
    FrameData frame{.x = static_cast<float>(pos_dist(gen)),
                    .y = static_cast<float>(pos_dist(gen)),
                    .z = static_cast<float>(pos_dist(gen)),
                    .timestamp = timestamp++};

    if (!frame_stream.push(frame)) {
      std::cout << "Warning: Stream is full, dropping frame" << std::endl;
    }

    std::cout << "Pushed frame: x=" << std::fixed << std::setprecision(2) << frame.x
              << ", y=" << frame.y << ", z=" << frame.z << ", ts=" << frame.timestamp << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  return 0;
}