#include "modbusController.h"
#include <atomic>
#include <csignal>
#include <format>
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

    std::cout << std::format("Movendo:    LEFT={} RIGHT={} UP={} DOWN={}\n",
                             state.isMoving[0],
                             state.isMoving[1],
                             state.isMoving[2],
                             state.isMoving[3]);

    std::cout << std::format("Preset:     P1={} P2={} P3={} P4={} | Setando={}\n",
                             state.readingPreset[0],
                             state.readingPreset[1],
                             state.readingPreset[2],
                             state.readingPreset[3],
                             state.isSettingPreset);

    std::cout << std::format("Calibrando: {}\n\n", state.isCalibrating);
  }

  return 0;
}
