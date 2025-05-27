#include "bmp280Controller.h"
#include "modbusController.h"
#include <csignal>
#include <iostream>
#include <ostream>
#include <sstream>
#include <unistd.h>

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

int main() {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);

  std::cout << "\033[2J\033[H";
  std::cout.flush();

  while (true) {
    auto screenState = modbus.readRegisters();
    auto sensorState = bmp280.readData();

    std::cout << "\033[H\033[J";

    std::ostringstream output;
    output << screenState;
    output << "Temperature: " << sensorState.temperature << "ºC\n";
    output << "Pressure: " << sensorState.pressure << "hPA\n";

    std::cout << output.str();
    std::cout.flush();

    modbus.write(ModbusController::SubCode::REG_TEMPERATURE, sensorState.temperature);
    modbus.write(ModbusController::SubCode::REG_PRESSURE, sensorState.pressure);
  }
  return 0;
}
