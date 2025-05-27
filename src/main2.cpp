#include "bmp280Controller.h"
#include "modbusController.h"
#include <csignal>
#include <cstdint>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

ModbusController modbus("/dev/serial0", B115200);
bmp280Controller bmp280;

using SubCode = ModbusController::SubCode;
using namespace std;

void closeProgram(int signal) {
    modbus.ensureClosed();
    bmp280.close();
    exit(0);
}

void updateBMP280(){
  bmp280Data bmpData = bmp280.readData();
  cout << "Temperature: " << bmpData.temperature << endl;
  cout << "Pressure: " << bmpData.pressure << endl;

  modbus.requestWrite(
      SubCode::REG_TEMPERATURE,
      std::span<uint8_t>(reinterpret_cast<uint8_t*>(&bmpData.temperature), sizeof(float))
  );

  modbus.requestWrite(
      SubCode::REG_PRESSURE,
      std::span<uint8_t>(reinterpret_cast<uint8_t*>(&bmpData.pressure), sizeof(float))
  );
}

int main() {
    std::signal(SIGINT, closeProgram);
    std::cout << "Programa rodando... (Ctrl+C para sair)\n\n";

    while (true) {
        updateBMP280();
    }
    return 0;
}

