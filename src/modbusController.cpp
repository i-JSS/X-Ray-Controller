#include "modbusController.h"
#include "uartController.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <stdexcept>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

#ifdef DEBUG
#include <iomanip>
#include <iostream>
std::string printHex(std::span<const uint8_t> data) {
  std::ostringstream oss;
  for (const auto &byte : data) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
  }
  return oss.str();
}

#endif

vector<uint8_t> ModbusController::WriteMessage::build() const {
  vector<uint8_t> msg = {ESP_ADDRESS,
                         static_cast<uint8_t>(Code::WRITE),
                         static_cast<uint8_t>(writeRegister),
                         static_cast<uint8_t>(data.size())};
  msg.insert(msg.end(), data.begin(), data.end());
  addPostfix(msg);
  return msg;
}

vector<uint8_t> ModbusController::ReadMessage::build() const {
  vector<uint8_t> msg = {ESP_ADDRESS,
                         static_cast<uint8_t>(Code::READ),
                         static_cast<uint8_t>(readRegister),
                         registerCount};
  addPostfix(msg);
  return msg;
}

void ModbusController::addPostfix(vector<uint8_t> &buffer) {
  buffer.insert(buffer.end(), MATRICULA.begin(), MATRICULA.end());

  uint16_t crc = calculateCRC(buffer.data(), buffer.size());
  buffer.push_back(static_cast<uint8_t>(crc & 0xFF));
  buffer.push_back(static_cast<uint8_t>(crc >> 8));
}

short ModbusController::CRC16(short crc, char data) {
  return ((crc & 0xFF00) >> 8) ^ tbl[(crc & 0x00FF) ^ (data & 0x00FF)];
}

short ModbusController::calculateCRC(const unsigned char *commands,
                                     const int size) {
  short crc = 0;
  for (int i = 0; i < size; i++)
    crc = CRC16(crc, commands[i]);
  return crc;
}

bool ModbusController::isValidCRC(const unsigned char *buffer, int length) {
  if (length < 3)
    return false;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
  return calculateCRC(buffer, length - 2) == buffer[length - 2] |
         (buffer[length - 1] << 8);
#pragma GCC diagnostic pop
}

vector<uint8_t> ModbusController::makeRequest(Message &message) {
  uart_.ensureOpen();
  uart_.send(message.build());
  uart_.sync();

  auto response = uart_.read(256);

  if (!isValidCRC(response.data(), response.size())) {
    uart_.ensureClosed();
    throw std::runtime_error("Invalid CRC checksum");
  }

  uart_.ensureClosed();
  return response;
}

ModbusController::RegisterState ModbusController::readRegisters() {
  RegisterState state;
  ReadMessage readMessage(SubCode::MOVE_X_LEFT_RIGHT, 5);
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

  clearRegisters(SubCode::MOVE_X_LEFT_RIGHT, 5);
  return state;
}

void ModbusController::init() {
  clearRegisters(SubCode::MOVE_X_LEFT_RIGHT, 30);
}

void ModbusController::clearRegisters(SubCode espRegister, int bytesToClear) {
  std::vector<uint8_t> clearData(bytesToClear, 0);
  write(espRegister, span(clearData));
}

void ModbusController::write(SubCode espRegister, span<const uint8_t> data) {
  WriteMessage writeMessage(espRegister, data);
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
