#include "modbusController.h"
#include "uartController.h"
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

vector<uint8_t> ModbusController::createReadMsg(SubCode subcode) {
  vector<uint8_t> msg = {ESP_ADDRESS, static_cast<uint8_t>(Code::READ),
                         static_cast<uint8_t>(subcode), 0x01};
  msg.insert(msg.end(), MATRICULA.begin(), MATRICULA.end());

  uint16_t crc = calculateCRC(msg.data(), msg.size());

  msg.push_back(crc);
  msg.push_back(crc >> 8);

  return msg;
}

void ModbusController::requestRead(SubCode subcode) {
  uart_.ensureOpen();
  while (true) {
    // PERF: alocar a mensagem todas as vezes pode ser c
    vector<uint8_t> msg = createReadMsg(subcode);
    bool res = uart_.send(msg);
    if (!res) {
      cerr << "Failed to send modbus message" << endl;
      continue;
    }

    array<uint8_t, 256> buffer;
    ssize_t len = uart_.read_into(buffer);
    if (len < 0) {
      cerr << "Failed to read modbus response" << endl;
      continue;
    }

    vector<uint8_t> resposta(buffer.begin(), buffer.begin() + len);
    if (isValidCRC(resposta.data(), resposta.size())) {
      break;
    } else {
      cerr << "Invalid checksum" << endl;
    }
  }
  uart_.ensureClosed();
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
