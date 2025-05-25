#include "modbusController.h"
#include "uartController.h"
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

void ModbusController::createMsg(Code code, SubCode subcode,
                                 vector<uint8_t> &msg,
                                 span<uint8_t> data = {}) {
  msg.clear();
  // 1 byte ESP, 2 byes code e subcode, 1 byte tamanho, n bytes dados,
  // matricula, 2 bytes crc
  msg.reserve(4 * sizeof(uint8_t) + data.size() + MATRICULA.size() + 2);
  msg = {ESP_ADDRESS, static_cast<uint8_t>(code),
         static_cast<uint8_t>(subcode)};
  if (data.empty()) {
    msg.push_back(0x01);
  } else {
    msg.push_back(data.size());
    msg.insert(msg.end(), data.begin(), data.end());
    msg.insert(msg.end(), MATRICULA.begin(), MATRICULA.end());
  }

  uint16_t crc = calculateCRC(msg.data(), msg.size());
  msg.push_back(crc);
  msg.push_back(crc >> 8);
}

// NOTE: inlining aqui faria sentido
vector<uint8_t> ModbusController::createReadMsg(SubCode subcode) {
  vector<uint8_t> msg;
  createMsg(Code::READ, subcode, msg);
  return msg;
}

vector<uint8_t> ModbusController::createWriteMsg(SubCode subcode,
                                                 span<uint8_t> data) {
  vector<uint8_t> msg;
  createMsg(Code::WRITE, subcode, msg, data);
  return msg;
}

static void printHex(const vector<uint8_t> &data) {
  for (uint8_t byte : data)
    printf("%02X ", byte);
  printf("\n");
}

uint32_t ModbusController::makeRequest(Code code, SubCode subcode,
                                       span<uint8_t> data) {
  vector<uint8_t> msg;
  createMsg(code, subcode, msg, data);
  uart_.ensureOpen();
  while (true) {
    bool res = uart_.send(msg);
    if (!res) {
      cerr << "Failed to send modbus message" << endl;
      continue;
    }

    vector<uint8_t> answer(256);
    if (uart_.read_into(answer, 256) < 0) {
      cerr << "Failed to read modbus response" << endl;
      continue;
    }

    if (isValidCRC(answer.data(), answer.size())) {
      uart_.ensureClosed();
#ifdef Debug
      printhex(answer);
#endif
      return answer[2];
    } else {
      cerr << "Invalid checksum" << endl;
      continue;
    }
  }
  uart_.ensureClosed();
  return -1; // Não deve chegar aqui
}

uint32_t ModbusController::requestRead(SubCode subcode) {
  return makeRequest(Code::READ, subcode);
}

uint32_t ModbusController::requestWrite(SubCode subcode, span<uint8_t> data) {
  return makeRequest(Code::WRITE, subcode, data);
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
  return calculateCRC(buffer, length - 2) == buffer[length - 2] |
         (buffer[length - 1] << 8);
}
