#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std;

constexpr array<uint8_t, 4> MATRICULA = {8, 1, 5, 0};

constexpr array<uint8_t, 4> NUM = {7, 7, 7, 7};

constexpr uint8_t MODBUS_ADDRESS = 0x01;

enum class ModbusCommand : uint8_t {
  READ = 0x23,
  WRITE = 0x16,
};

enum class Subcode : uint8_t {
  INT_REQUEST = 0xA1,
  FLOAT_REQUEST = 0xA2,
  STRING_REQUEST = 0xA3,
  INT_SEND = 0xB1,
  FLOAT_SEND = 0xB2,
  STRING_SEND = 0xB3
};

class UARTController {
public:
  UARTController(string portName, const speed_t baudRate)
      : portName(move(portName)), baudRate(baudRate) {}

  void requestInt() {
    int fd = open_serial();

    vector<uint8_t> msg = {
        MODBUS_ADDRESS,
        static_cast<uint8_t>(ModbusCommand::READ),
        static_cast<uint8_t>(Subcode::INT_REQUEST),
    };
    msg.insert(msg.end(), MATRICULA.begin(), MATRICULA.end());

    uint16_t crc = calcula_CRC(msg.data(), msg.size());

    msg.push_back(crc);
    msg.push_back((crc >> 8));
    cout << "Enviando: ";
    printHex(msg);

    ssize_t bytes_written = write(fd, msg.data(), msg.size());
    if (bytes_written < 0) {
      cerr << "Erro ao enviar dados.\n";
      close_serial(fd);
      return;
    }

    fsync(fd);
    usleep(100000);

    uint8_t buffer[256];
    ssize_t len = read(fd, buffer, sizeof(buffer));
    if (len < 0) {
      cerr << "Erro na leitura da resposta.\n";
      close_serial(fd);
      return;
    }

    vector<uint8_t> resposta(buffer, buffer + len);

    cout << "Resposta recebida: ";
    printHex(resposta);

    uint32_t valor = (resposta[3] << 24) | (resposta[4] << 16) |
                     (resposta[5] << 8) | (resposta[6]);
    cout << "Valor inteiro: " << valor << endl;

    close_serial(fd);
  }

  void sendInt() {
    int fd = open_serial();

    vector<uint8_t> msg = {
        MODBUS_ADDRESS,
        static_cast<uint8_t>(ModbusCommand::WRITE),
        static_cast<uint8_t>(Subcode::INT_SEND),
    };
    msg.insert(msg.end(), NUM.begin(), NUM.end());
    msg.insert(msg.end(), MATRICULA.begin(), MATRICULA.end());

    uint16_t crc = calcula_CRC(msg.data(), msg.size());

    msg.push_back(crc & 0xFF);
    msg.push_back((crc >> 8) & 0xFF);
    cout << "Enviando: ";
    printHex(msg);

    ssize_t bytes_written = write(fd, msg.data(), msg.size());
    if (bytes_written < 0) {
      cerr << "Erro ao enviar dados.\n";
      close_serial(fd);
      return;
    }

    fsync(fd);
    usleep(100000);

    uint8_t buffer[256];
    ssize_t len = read(fd, buffer, sizeof(buffer));
    if (len < 0) {
      cerr << "Erro na leitura da resposta.\n";
      close_serial(fd);
      return;
    }

    vector<uint8_t> resposta(buffer, buffer + len);

    cout << "Resposta recebida: ";
    printHex(resposta);

    uint32_t valor = (resposta[3] << 24) | (resposta[4] << 16) |
                     (resposta[5] << 8) | (resposta[6]);
    cout << "Valor inteiro: " << valor << endl;

    close_serial(fd);
  }

private:
  string portName;
  speed_t baudRate;

  int open_serial() {
    int fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1) {
      perror("Erro ao abrir UART");
      return -1;
    }

    termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
      perror("Erro ao obter atributos da UART");
      close(fd);
      return -1;
    }

    cfsetospeed(&tty, baudRate);
    cfsetispeed(&tty, baudRate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      perror("Erro ao aplicar configurações na UART");
      close(fd);
      return -1;
    }

    return fd;
  }

  void close_serial(int fd) {
    if (fd >= 0)
      close(fd);
  }

  short CRC16(short crc, char data) {
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

  short calcula_CRC(const unsigned char *commands, const int size) {
    short crc = 0;
    for (int i = 0; i < size; i++)
      crc = CRC16(crc, commands[i]);
    return crc;
  }

  void printHex(const vector<uint8_t> &data) {
    for (uint8_t byte : data)
      printf("%02X ", byte);
    printf("\n");
  }
};

int main() {
  UARTController uart("/dev/serial0", B115200);
  uart.requestInt();
  uart.sendInt();
  return 0;
}
