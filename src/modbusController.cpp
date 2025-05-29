#include "modbusController.h"
#include "uartController.h"
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

#ifdef DEBUG
#include <iomanip>
#include <sstream>
#include <unordered_map>
static void printHex(std::span<uint8_t> data) {
  std::ostringstream oss;
  for (uint8_t byte : data) {
    oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
  }
  std::cout << oss.str() << std::endl;
}
#endif

vector<uint8_t> ModbusController::makeRequest(Message &message) {
  uart_.ensureOpen();
  while (true) {
    try {
      uart_.send(message.build());
      uart_.sync();

      auto response = uart_.read(256);

#ifdef DEBUG
      cout << "Resposta recebida: ";
      printHex(response);
#endif
      if (!isValidCRC(response.data(), response.size()))
        throw std::runtime_error("Invalid CRC checksum");

      uart_.ensureClosed();
      return response;
    } catch (const std::exception &e) {
      cerr << "Erro ao fazer requisição: " << e.what() << endl;
      continue;
    }
  }
}

ModbusController::RegisterState ModbusController::readRegisters() {
  RegisterState state;
  ReadMessage readMessage;

  readMessage.readRegister = SubCode::MOVE_X_LEFT_RIGHT;
  readMessage.registerCount = 5;
  auto response = makeRequest(readMessage);
  int offset = 2;

  state.isMoving[0] = response[offset] & 0x01;
  state.isMoving[1] = response[offset] & 0x02;

  // Próximo
  offset++;

  state.isMoving[2] = response[offset] & 0x01;
  state.isMoving[3] = response[offset] & 0x02;

  offset++;

  state.readingPreset[0] = response[offset] & 0x01;
  state.readingPreset[1] = response[offset] & 0x02;
  state.readingPreset[2] = response[offset] & 0x04;
  state.readingPreset[3] = response[offset] & 0x08;

  state.isCalibrating = response[offset++];
  state.isSettingPreset = response[offset++];

  // Limpar registradores de leitura
  clearRegisters(SubCode::MOVE_X_LEFT_RIGHT, 5);
  return state;
}

void ModbusController::init() {
  // Zerar todos os registradores
  clearRegisters(SubCode::MOVE_X_LEFT_RIGHT, 30);
}

void ModbusController::clearRegisters(SubCode espRegister, int bytesToClear) {
  std::vector<uint8_t> clearData(bytesToClear, 0);
  write(espRegister, span(clearData));
}

void ModbusController::write(SubCode espRegister, span<const uint8_t> data) {
  WriteMessage writeMessage;
  writeMessage.writeRegister = espRegister;
  writeMessage.data = data;

  makeRequest(writeMessage);
}

void ModbusController::write(SubCode espRegister, float value) {
  uint8_t *dataPtr = reinterpret_cast<uint8_t *>(&value);
  write(espRegister, span(dataPtr, sizeof(float)));
}

void ModbusController::write(SubCode espRegister, byte value) {
  uint8_t valueAsUint8 = static_cast<uint8_t>(value);
  write(espRegister, span(&valueAsUint8, 1));
}
