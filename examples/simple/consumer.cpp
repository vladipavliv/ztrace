#include <chrono>
#include <iostream>
#include <thread>
#include <ztrace/ztrace.hpp>

int main() {
  try {
    ztrace::init();

    ztrace::Viewer<double> fps_viewer("fps");
    ztrace::Viewer<int> players_viewer("players");

    std::cout << "Consumer started. Reading every 1s..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    while (true) {
      double fps = fps_viewer.read();
      int players = players_viewer.read();

      std::cout << "[Consumer] FPS: " << fps << ", Players: " << players << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}