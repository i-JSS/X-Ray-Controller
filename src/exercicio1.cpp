#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <utility>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

enum class Command {
  SOLICITA_INT = 0xA1,
  SOLICITA_FLOAT = 0xA2,
  SOLICITA_STRING = 0xA3,
};

class Uart {
  Uart(const string &portName, speed_t baudRate)
      : portName(portName), baudRate(baudRate) {
    fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
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
    array<uint8_t, 5> msg = {uint8_t(comando), 8, 1, 5, 0};
    if (write(fd, msg.data(), msg.size()) != 5)
      throw std::system_error(errno, std::generic_category(), "Failed writing to UART port");

    fsync(fd);
    usleep(100000);

    switch (comando) {
      case Command::SOLICITA_INT: {
        int valor = 0;
        int lido = 0;
        while (lido < 4) {
          int r = read(fd, ((unsigned char *)&valor) + lido, 4 - lido);
          if (r > 0)
            lido += r;
        }
        cout << "Valor recebido (int): " << valor << endl;
        break;
      }
      case Command::SOLICITA_FLOAT: {
        float valor = 0;
        int lido = 0;
        while (lido < 4) {
          int r = read(fd, ((unsigned char *)&valor) + lido, 4 - lido);
          if (r > 0)
            lido += r;
        }
        cout << "Valor recebido (float): " << valor << endl;
        break;
      }
      case Command::SOLICITA_STRING: {
        uint8_t len;
        if (read(fd, &len, 1) != 1)
          throw runtime_error("Erro ao ler tamanho da string");

        string str(len, '\0');
        int lido = 0;
        while (lido < len) {
          int r = read(fd, &str[lido], len - lido);
          if (r > 0)
            lido += r;
        }
        cout << "String recebida: " << str << endl;
        break;
      }
    if (fd >= 0) close(fd);
  }

private:
  string portName;
  speed_t baudRate;
  int fd = -1;
};

int main() {
  Uart uart("/dev/serial0", B9600);
  uart.request(Command::SOLICITA_INT);
  uart.request(Command::SOLICITA_FLOAT);
  uart.request(Command::SOLICITA_STRING);

  return 0;
}
