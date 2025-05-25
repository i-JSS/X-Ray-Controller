#pragma once
#include "modbusController.h"
#include "uartController.h"
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

class ModbusController {
public:
  enum class Code : uint8_t {
    READ = 0x03,
    WRITE = 0x06,
  };

  enum class SubCode : uint8_t {
    MOVE_X_LEFT_RIGHT = 0x00,
    MOVE_Y_UP_DOWN = 0x01,
    PRESET_POSITIONS = 0x02,
    SET_PRESET_POSITION = 0x03,
    CALIBRATE = 0x04,
    REG_SPEED_X = 0x05,
    REG_SPEED_Y = 0x09,
    REG_POSITION_X = 0x0D,
    REG_POSITION_Y = 0x11,
    REG_TEMPERATURE = 0x15,
    REG_PRESSURE = 0x19,
    REG_MACHINE_STATE = 0x1D
  };

  explicit ModbusController(const string &portName, const speed_t baudRate)
      : uart_(portName, baudRate) {}

  [[nodiscard]] uint32_t requestRead(SubCode subcode);
  [[nodiscard]] uint32_t requestWrite(SubCode subcode, span<uint8_t> data);

  inline void ensureClosed() { uart_.ensureClosed(); }

private:
  uint32_t makeRequest(Code code, SubCode subcode, span<uint8_t> data = {});
  vector<uint8_t> createMsg(Code code, SubCode subcode, span<uint8_t> data);
  vector<uint8_t> createReadMsg(SubCode subcode);
  vector<uint8_t> createWriteMsg(SubCode subcode, span<uint8_t> data);
  UARTController uart_;

  static constexpr uint8_t ESP_ADDRESS = 0x01;
  static constexpr array<uint8_t, 4> MATRICULA = {8, 1, 5, 0};

  short CRC16(short crc, char data);
  short calculateCRC(const unsigned char *commands, const int size);
  bool isValidCRC(const unsigned char *buffer, int length);
};
