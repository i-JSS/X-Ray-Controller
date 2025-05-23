#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

enum Command {
  SOLICITA_INT = 0xA1,
  SOLICITA_FLOAT = 0xA2,
  SOLICITA_STRING = 0xA3,
};

class Uart {
public:
  Uart(const string &portName, speed_t baudRate)
      : portName(portName), baudRate(baudRate) {
    int fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1)
      throw std::system_error(errno, std::generic_category(),
                              "Failed to open UART port");

    termios tty{};
    if (tcgetattr(fd, &tty) != 0)
      throw std::system_error(errno, std::generic_category(),
                              "Failed receiving UART attributes");

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

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
      throw std::system_error(errno, std::generic_category(),
                              "Failed applying UART attributes");
  }

  void request(Command comando) {
    array<byte, 5> msg = {byte(comando), byte(8), byte(1), byte(5), byte(0)};
    if (write(fd, &msg, 5) != 5)
      throw std::system_error(errno, std::generic_category(),
                              "Failed writing to UART port");

    fsync(fd);
    usleep(100000);

    switch (comando) {
    case SOLICITA_INT: {
      int valor = 0;
      int lido = 0;
      while (lido < 4) {
        int r = read(fd, ((unsigned char *)&valor) + lido, 4 - lido);
        if (r > 0)
          lido += r;
      }
      std::cout << "Valor recebido (int): " << valor << std::endl;
    } break;
    case SOLICITA_FLOAT: {
      float valor = 0;
      int lido = 0;
      while (lido < 4) {
        int r = read(fd, ((unsigned char *)&valor) + lido, 4 - lido);
        if (r > 0)
          lido += r;
      }
      std::cout << "Valor recebido (float): " << valor << std::endl;
    } break;
    case SOLICITA_STRING: {
      uint8_t len;
      read(fd, &len, 1);
      std::string str(len + 1, '\0');
      int lido = 0;
      while (lido < len) {
        int r = read(fd, &str[lido], len - lido);
        if (r > 0)
          lido += r;
      }
      std::cout << "String recebida: " << str << std::endl;
    } break;
    }
  }

  ~Uart() {
    if (fd != -1) {
      close(fd);
    }
  }

private:
  string portName;
  speed_t baudRate;
  int fd = -1;
};

int main() {
  Uart uart("/dev/serial0", B9600);
  uart.request(SOLICITA_INT);
  uart.request(SOLICITA_FLOAT);
  uart.request(SOLICITA_STRING);

  return 0;
}
