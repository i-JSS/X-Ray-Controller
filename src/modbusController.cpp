#include "modbusController.h"
#include "easylogging++.h"
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

std::vector<uint8_t> ModbusController::WriteMessage::build() const {
  std::vector<uint8_t> msg = {ESP_ADDRESS,
                              static_cast<uint8_t>(Code::WRITE),
                              static_cast<uint8_t>(writeRegister),
                              static_cast<uint8_t>(data.size())};
  msg.insert(msg.end(), data.begin(), data.end());
  addPostfix(msg);
  return msg;
}

std::vector<uint8_t> ModbusController::ReadMessage::build() const {
  std::vector<uint8_t> msg = {ESP_ADDRESS,
                              static_cast<uint8_t>(Code::READ),
                              static_cast<uint8_t>(readRegister),
                              registerCount};
  addPostfix(msg);
  return msg;
}

void ModbusController::addPostfix(std::vector<uint8_t> &buffer) {
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

std::vector<uint8_t> ModbusController::makeRequest(Message &message) {
  auto builtMessage = message.build();
  LOG_EVERY_N(DEBUG_N, DEBUG) << "Sending Modbus message: " << toHexString(builtMessage);
  uart_.ensureOpen();
  uart_.send(builtMessage);
  uart_.sync();

  LOG_EVERY_N(DEBUG_N, DEBUG) << "Message sent successfully, waiting for response...";

  auto response = uart_.read(256);

  LOG_EVERY_N(DEBUG_N, DEBUG) << "Received Modbus response";

  if (!isValidCRC(response.data(), response.size())) {
    uart_.ensureClosed();
    throw std::runtime_error("Invalid CRC checksum: " +
                             toHexString(std::span(response)));
  }

  LOG_EVERY_N(DEBUG_N, DEBUG) << "CRC checksum is valid";
  uart_.ensureClosed();
  return response;
}

std::ostream &operator<<(std::ostream &os, const ModbusController::RegisterState &state) {
  os << "RegisterState { ";

  os << "isMoving: [";
  for (size_t i = 0; i < state.isMoving.size(); ++i) {
    os << (state.isMoving[i] ? "true" : "false");
    if (i < state.isMoving.size() - 1)
      os << ", ";
  }
  os << "], ";

  os << "selectedPreset: ";
  if (state.selectedPreset.has_value()) {
    os << state.selectedPreset.value();
  } else {
    os << "none";
  }
  os << ", ";

  os << "isCalibrating: " << (state.isCalibrating ? "true" : "false") << ", ";
  os << "isSettingPreset: " << (state.isSettingPreset ? "true" : "false");

  os << " }";
  return os;
}

ModbusController::RegisterState ModbusController::readRegisters() {
  LOG_EVERY_N(DEBUG_N, DEBUG) << "Reading registers from Modbus controller";
  RegisterState state;
  ReadMessage readMessage(SubCode::MOVE_X, 5);
  auto response = makeRequest(readMessage);
  int offset = 2;

  int moveByte = (response[offset + 1] << 2) | response[offset];
  offset += 2;
  uint8_t presetByte = response[offset++];
  for (int i = 0; i < 4; i++) {
    int currentMask = 1 << i;
    state.isMoving[i] = moveByte & currentMask;
    if (presetByte & currentMask)
      state.selectedPreset = i + 1;
  }

  state.isSettingPreset = response[offset++];
  state.isCalibrating = response[offset++];
  LOG_EVERY_N(DEBUG_N, DEBUG) << "Registers read successfully: " << state;
  clearRegisters(SubCode::MOVE_X, 5);
  return state;
}

void ModbusController::init() {
  clearRegisters(SubCode::MOVE_X, 30);
}

void ModbusController::clearRegisters(SubCode espRegister, int bytesToClear) {
  LOG_EVERY_N(DEBUG_N, DEBUG) << "Clearing " << bytesToClear << " bytes for ESP register: "
                              << static_cast<int>(espRegister);

  std::vector<uint8_t> clearData(bytesToClear, 0);
  write(espRegister, std::span(clearData));
  LOG_EVERY_N(DEBUG_N, DEBUG) << "Registers cleared successfully";
}

void ModbusController::write(SubCode espRegister, std::span<const uint8_t> data) {
  LOG_EVERY_N(DEBUG_N, DEBUG) << "Writing " << data.size() << " bytes to ESP register: "
                              << static_cast<int>(espRegister);

  WriteMessage writeMessage(espRegister, data);
  makeRequest(writeMessage);

  LOG_EVERY_N(DEBUG_N, DEBUG) << "Data written successfully to ESP register: ";
}

void ModbusController::write(SubCode espRegister, float value) {
  uint8_t *dataPtr = reinterpret_cast<uint8_t *>(&value);
  write(espRegister, std::span(dataPtr, sizeof(float)));
}

void ModbusController::write(SubCode espRegister, std::byte value) {
  uint8_t valueAsUint8 = static_cast<uint8_t>(value);
  write(espRegister, std::span(&valueAsUint8, 1));
}
