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
  int offset = 3;

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

  return state;
}

// Escrever fora da região de memória dá erro
void ModbusController::write(SubCode espRegister, float value) {
  WriteMessage writeMessage;
  writeMessage.writeRegister = espRegister;

  uint8_t *dataPtr = reinterpret_cast<uint8_t *>(&value);
  writeMessage.data.assign(dataPtr, dataPtr + sizeof(float));

  makeRequest(writeMessage);
}

void ModbusController::write(SubCode espRegister, byte value) {
  WriteMessage writeMessage;
  writeMessage.writeRegister = espRegister;
  writeMessage.data.push_back(static_cast<uint8_t>(value));

  makeRequest(writeMessage);
}
