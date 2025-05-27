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

static unordered_map<ModbusController::Code, string> codeToString = {
    {ModbusController::Code::READ, "READ"},
    {ModbusController::Code::WRITE, "WRITE"},
};
static unordered_map<ModbusController::SubCode, string> subcodeToString = {
    {ModbusController::SubCode::MOVE_X_LEFT_RIGHT, "MOVE_X_LEFT_RIGHT"},
    {ModbusController::SubCode::MOVE_Y_UP_DOWN, "MOVE_Y_UP_DOWN"},
    {ModbusController::SubCode::PRESET_POSITIONS, "PRESET_POSITIONS"},
    {ModbusController::SubCode::SET_PRESET_POSITION, "SET_PRESET_POSITION"},
    {ModbusController::SubCode::CALIBRATE, "CALIBRATE"},
    {ModbusController::SubCode::REG_SPEED_X, "REG_SPEED_X"},
    {ModbusController::SubCode::REG_SPEED_Y, "REG_SPEED_Y"},
    {ModbusController::SubCode::REG_POSITION_X, "REG_POSITION_X"},
    {ModbusController::SubCode::REG_POSITION_Y, "REG_POSITION_Y"},
    {ModbusController::SubCode::REG_TEMPERATURE, "REG_TEMPERATURE"},
    {ModbusController::SubCode::REG_PRESSURE, "REG_PRESSURE"},
    {ModbusController::SubCode::REG_MACHINE_STATE, "REG_MACHINE_STATE"},
};

static void printMessageDebug(span<uint8_t> message) {
  uint8_t espAddress = message[0];
  uint8_t code = message[1];
  uint8_t subcode = message[2];
  uint8_t dataLength = message[3];

  int dataOffset = 4; // Skip header 4 bytes (address, code, subcode, length)
  if (code == static_cast<uint8_t>(ModbusController::Code::WRITE)) {
    dataOffset += dataLength; // Skip actual data bytes
  }
  dataOffset += 4; // MATRICULA 4 bytes

  uint16_t crc = (message[dataOffset] | (message[dataOffset + 1] << 8));

  span<uint8_t> data;
  if (code == static_cast<uint8_t>(ModbusController::Code::WRITE) && dataLength > 0) {
    data = message.subspan(4, dataLength); // Data starts after header
  }

  std::ostringstream dataStr;
  if (data.empty()) {
    dataStr << "N/A";
  } else {
    for (uint8_t byte : data) {
      dataStr << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
  }

  std::ostringstream payloadStr;
  for (uint8_t byte : message) {
    payloadStr << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
  }

  std::cout << "\n --- Mensagem criada ---- \n"
            << "Código: " << codeToString[static_cast<ModbusController::Code>(code)] << "\n"
            << "Subcódigo: " << subcodeToString[static_cast<ModbusController::SubCode>(subcode)] << "\n"
            << "Dados: " << dataStr.str() << "\n"
            << "CRC: " << std::hex << std::setw(2) << std::setfill('0') << (crc & 0xFF) << " "
            << std::setw(2) << std::setfill('0') << (crc >> 8) << "\n"
            << "Payload: " << payloadStr.str() << "\n"
            << "--- Fim da mensagem ----\n";
}
#endif

vector<uint8_t> ModbusController::makeRequest(Message &message) {
  uart_.ensureOpen();
  while (true) {
    try {
      uart_.send(message.build());
      // NOTE: dá pra pegar o tamanho esperado da resposta exato
      // e dá pra conferir se o tamanho bate
      auto response = uart_.read(256);

      uart_.sync();

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

  for (int i = offset; i < offset + 4; i++) {
    state.isMoving[i - offset] = response[i];
  }
  offset += 4;

  for (int i = offset; i < offset + 4; i++) {
    state.readingPreset[i - offset] = response[i];
  }
  offset += 4;

  state.isCalibrating = response[offset++];
  state.isSettingPreset = response[offset++];

  return state;
}

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
