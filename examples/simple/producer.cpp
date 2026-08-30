#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <ztrace/ztrace.hpp>

int main() {
  try {
    ztrace::init();

    const int32_t update_rate_hz = 100;

    ztrace::Variable<double> fps("fps", update_rate_hz, 0.0, ztrace::MemoryOrder::Relaxed);
    ztrace::Variable<int> players("players", update_rate_hz, 0, ztrace::MemoryOrder::Relaxed);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> fps_dist(30.0, 60.0);
    std::uniform_int_distribution<> player_dist(1, 100);

    std::cout << "Producer started. Updating and logging every 1s..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    while (true) {
      double current_fps = fps_dist(gen);
      int current_players = player_dist(gen);

      fps.update(current_fps);
      players.update(current_players);

      /*      std::cout << "[Producer] FPS: " << current_fps << ", Players: " << current_players
                      << std::endl;*/

      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / update_rate_hz));
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}