#include "modbusController.h"
#include <csignal>
#include <iostream>

ModbusController modbus("/dev/serial0", B115200);
void handler(int) {
  modbus.ensureClosed();
  exit(0);
}

int main() {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);

  while (true) {
    auto state = modbus.readRegisters();

    std::cout << "\033[3A";
    std::cout << "\033[2K\r";
    std::cout << "Movendo:    LEFT=" << state.isMoving[0]
              << " RIGHT=" << state.isMoving[1]
              << " UP=" << state.isMoving[2]
              << " DOWN=" << state.isMoving[3] << "\n";

    std::cout << "\033[2K\r";
    std::cout << "Preset:     P1=" << state.readingPreset[0]
              << " P2=" << state.readingPreset[1]
              << " P3=" << state.readingPreset[2]
              << " P4=" << state.readingPreset[3]
              << " | Setando=" << state.isSettingPreset << "\n";

    std::cout << "\033[2K\r";
    std::cout << "Calibrando: " << state.isCalibrating << "\n";
  }

  return 0;
}
