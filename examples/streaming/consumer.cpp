#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <ztrace/ztrace.hpp>

struct FrameData {
  float x;
  float y;
  float z;
  uint64_t timestamp;
};

int main() {
  try {
    ztrace::init();
    ztrace::Stream<FrameData> frame_stream("frames");

    std::cout << "Streaming consumer started. Reading frames..." << std::endl;

    while (true) {
      std::vector<FrameData> frames;
      frame_stream.drain(frames);

      if (!frames.empty()) {
        for (const auto &frame : frames) {
          std::cout << "Received frame:  x=" << std::fixed << std::setprecision(2) << frame.x
                    << ", y=" << frame.y << ", z=" << frame.z << ", ts=" << frame.timestamp
                    << std::endl;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}