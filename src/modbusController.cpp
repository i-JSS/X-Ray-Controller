#include "modbusController.h"
#include "uartController.h"
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#ifdef DEBUG
#include <iomanip>
#include <unordered_map>
#endif

using namespace std;

#ifdef DEBUG
static void printHex(const vector<uint8_t> &data) {
  for (uint8_t byte : data)
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(byte) << " ";
  std::cout << std::dec << std::endl;
}
#endif

vector<uint8_t> ModbusController::createMsg(Code code, SubCode subcode,
                                            span<uint8_t> data = {}) {

  vector<uint8_t> msg = {
      ESP_ADDRESS,
      static_cast<uint8_t>(code),
      static_cast<uint8_t>(subcode),
      data.empty() ? static_cast<uint8_t>(1) : static_cast<uint8_t>(data.size())};
  if (!data.empty())
    msg.insert(msg.end(), data.begin(), data.end());

  msg.insert(msg.end(), MATRICULA.begin(), MATRICULA.end());

  uint16_t crc = calculateCRC(msg.data(), msg.size());
  msg.push_back(static_cast<uint8_t>(crc & 0xFF));
  msg.push_back(static_cast<uint8_t>(crc >> 8));

#ifdef DEBUG
  static unordered_map<Code, string> codeToString = {
      {Code::READ, "READ"},
      {Code::WRITE, "WRITE"},
  };
  static unordered_map<SubCode, string> subcodeToString = {
      {SubCode::MOVE_X_LEFT_RIGHT, "MOVE_X_LEFT_RIGHT"},
      {SubCode::MOVE_Y_UP_DOWN, "MOVE_Y_UP_DOWN"},
      {SubCode::PRESET_POSITIONS, "PRESET_POSITIONS"},
      {SubCode::SET_PRESET_POSITION, "SET_PRESET_POSITION"},
      {SubCode::CALIBRATE, "CALIBRATE"},
      {SubCode::REG_SPEED_X, "REG_SPEED_X"},
      {SubCode::REG_SPEED_Y, "REG_SPEED_Y"},
      {SubCode::REG_POSITION_X, "REG_POSITION_X"},
      {SubCode::REG_POSITION_Y, "REG_POSITION_Y"},
      {SubCode::REG_TEMPERATURE, "REG_TEMPERATURE"},
      {SubCode::REG_PRESSURE, "REG_PRESSURE"},
      {SubCode::REG_MACHINE_STATE, "REG_MACHINE_STATE"},
  };

  cout << "\n --- Mensagem criada ---- \n"
       << "Código: " << codeToString[code] << "\n"
       << "Subcódigo: " << subcodeToString[subcode] << "\n"
       << "Dados: " << (data.empty() ? "N/A" : "");
  for (uint8_t byte : data) {
    cout << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(byte) << " ";
  }
  cout << "\nMatrícula: ";
  for (uint8_t byte : MATRICULA) {
    cout << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(byte) << " ";
  }
  std::cout << "\nCRC: ";
  printHex({static_cast<uint8_t>(crc & 0xFF), static_cast<uint8_t>(crc >> 8)});
  std::cout << "Payload: ";
  printHex(msg);
  std::cout << "--- Fim da mensagem ----\n";
#endif
  return msg;
}

vector<uint8_t> ModbusController::createReadMsg(SubCode subcode) {
  return createMsg(Code::READ, subcode);
}

vector<uint8_t> ModbusController::createWriteMsg(SubCode subcode,
                                                 span<uint8_t> data) {
  return createMsg(Code::WRITE, subcode, data);
}

uint32_t ModbusController::makeRequest(Code code, SubCode subcode,
                                       span<uint8_t> data) {
  vector<uint8_t> msg = createMsg(code, subcode, data);
  uart_.ensureOpen();

  while (true) {
    try {
      uart_.send(msg);
      auto response = uart_.read(256);

      if (!isValidCRC(response.data(), response.size()))
        throw std::runtime_error("Invalid CRC checksum");

      uart_.ensureClosed();
      return response[2];
    } catch (const std::system_error &e) {
      cerr << "Erro ao fazer requisição: " << e.what() << endl;
      continue;
    }
  }
}

uint32_t ModbusController::requestRead(SubCode subcode) {
  return makeRequest(Code::READ, subcode);
}

uint32_t ModbusController::requestWrite(SubCode subcode, span<uint8_t> data) {
  return makeRequest(Code::WRITE, subcode, data);
}

short ModbusController::CRC16(short crc, char data) {
  const unsigned short tbl[256] = {
      0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601,
      0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0,
      0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81,
      0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941,
      0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01,
      0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0,
      0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081,
      0x1040, 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
      0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 0x3C00,
      0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0,
      0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1, 0xE981,
      0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41,
      0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700,
      0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0,
      0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281,
      0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
      0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01,
      0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1,
      0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0, 0x7F80,
      0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541,
      0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101,
      0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0,
      0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481,
      0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
      0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 0x8801,
      0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1,
      0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 0x4400, 0x84C1, 0x8581,
      0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341,
      0x4100, 0x81C1, 0x8081, 0x4040};
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
