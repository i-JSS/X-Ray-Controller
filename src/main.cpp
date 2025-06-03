#include "bmp280Controller.h"
#include "easylogging++.h"
#include "modbusController.h"
#include <csignal>
#include <iostream>
#include <ostream>
#include <unistd.h>

INITIALIZE_EASYLOGGINGPP

ModbusController modbus("/dev/serial0", B115200);
bmp280Controller bmp280;

void handler(int) {
  modbus.ensureClosed();
  bmp280.close();
  exit(0);
}

std::ostream &operator<<(ostream &os, const ModbusController::RegisterState &state) {
  os << "Movendo:    LEFT=" << state.isMoving[0]
     << " RIGHT=" << state.isMoving[1]
     << " UP=" << state.isMoving[2]
     << " DOWN=" << state.isMoving[3] << "\n";
  os << "Preset:     P1=" << state.readingPreset[0]
     << " P2=" << state.readingPreset[1]
     << " P3=" << state.readingPreset[2]
     << " P4=" << state.readingPreset[3]
     << " | Configurando?=" << state.isSettingPreset << "\n";
  os << "Calibrando?: " << state.isCalibrating << "\n";
  return os;
}

std::ostream &operator<<(ostream &os, const bmp280Data &state) {
  os << "Temperature: " << state.temperature << "ºC\n";
  os << "Pressure: " << state.pressure << "hPA\n";
  return os;
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);

  std::cout << "\033[2J\033[H";
  std::cout.flush();

  modbus.init();
  while (true) {
    try {
      auto sensorState = bmp280.readData();
      std::cout << modbus.readRegisters() << sensorState;

      modbus.write(ModbusController::SubCode::TEMP, sensorState.temperature);
      modbus.write(ModbusController::SubCode::PRESSURE, sensorState.pressure);
    } catch (const std::exception &e) {
#ifdef DEBUG
      std::cerr << "Error: " << e.what() << "\n";
#endif
      continue;
    }
  }
  return 0;
}
