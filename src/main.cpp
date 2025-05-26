#include "modbusController.h"
#include <atomic>
#include <csignal>
#include <iostream>

std::atomic<bool> running{true};
void handler(int) { running.store(false); }

int main() {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);

  ModbusController modbus("/dev/ttyUSB0", B115200);

  while (running.load()) {
    auto state = modbus.readRegisters();

    std::cout << "Movendo:    LEFT=" << state.isMoving[0]
              << " RIGHT=" << state.isMoving[1]
              << " UP=" << state.isMoving[2]
              << " DOWN=" << state.isMoving[3] << "\n";

    std::cout << "Preset:     P1=" << state.readingPreset[0]
              << " P2=" << state.readingPreset[1]
              << " P3=" << state.readingPreset[2]
              << " P4=" << state.readingPreset[3]
              << " | Setando=" << state.isSettingPreset << "\n";

    std::cout << "Calibrando: " << state.isCalibrating << "\n\n";
  }

  return 0;
}
