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
  ;

  struct RegisterState {
    array<bool, 4> isMoving = {0, 0, 0, 0};
    array<bool, 4> readingPreset = {0, 0, 0, 0};
    bool isCalibrating = false;
    bool isSettingPreset = false;
  };

  explicit ModbusController(const string &portName, const speed_t baudRate)
      : uart_(portName, baudRate) {}

  [[nodiscard]] RegisterState readRegisters();
  void write(SubCode espRegister, float value);
  void write(SubCode espRegister, byte value);

  void ensureClosed() { uart_.ensureClosed(); }

private:
  UARTController uart_;

  static constexpr uint8_t ESP_ADDRESS = 0x01;
  static constexpr array<uint8_t, 4> MATRICULA = {8, 1, 5, 0};

  short CRC16(short crc, char data);
  short calculateCRC(const unsigned char *commands, const int size);
  bool isValidCRC(const unsigned char *buffer, int length);

  struct Message {
    virtual vector<uint8_t> build() const = 0;
    virtual uint8_t getQtd() const = 0;
  };
  struct ReadMessage : Message {
    SubCode readRegister;
    uint8_t registerCount;

    vector<uint8_t> build() const override {
      return {ESP_ADDRESS,
              static_cast<uint8_t>(Code::READ),
              static_cast<uint8_t>(readRegister),
              registerCount};
    };

    uint8_t getQtd() const override { return registerCount; }
  };
  struct WriteMessage : Message {
    SubCode writeRegister;
    vector<uint8_t> data;

    vector<uint8_t> build() const override {
      vector<uint8_t> msg = {ESP_ADDRESS,
                             static_cast<uint8_t>(Code::WRITE),
                             static_cast<uint8_t>(writeRegister),
                             static_cast<uint8_t>(data.size())};
      msg.insert(msg.end(), data.begin(), data.end());
      return msg;
    }

    uint8_t getQtd() const override { return static_cast<uint8_t>(data.size()); }
  };

  vector<uint8_t> finalizeMessage(Message &message); // Write into Register
  vector<uint8_t> makeRequest(Message &message);
};
