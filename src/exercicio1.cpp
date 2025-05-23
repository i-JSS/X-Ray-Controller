#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

class Uart {
public:

  enum class Command : uint8_t {
    INT_REQUEST = 0xA1,
    FLOAT_REQUEST = 0xA2,
    STRING_REQUEST = 0xA3,
    INT_SEND = 0xB1,
    FLOAT_SEND = 0xB2,
    STRING_SEND = 0xB3,
  };

  Uart(const string &portName, speed_t baudRate)
      : portName(portName), baudRate(baudRate) {}

  int request(Command comando) {
    int fd = openSerial();
    array<uint8_t, 5> msg = {uint8_t(comando), 8, 1, 5, 0};
    if (write(fd, msg.data(), msg.size()) != 5) {
      throw std::system_error(errno, std::generic_category(), "Failed writing to UART port");
    }
    fsync(fd);
    usleep(100000);
    return fd;
  }

  void requestInt() {
    int fd = request(Command::INT_REQUEST), valor = 0, lido = 0;
    while (lido < 4) {
      int r = read(fd, ((unsigned char *)&valor) + lido, 4 - lido);
      if (r > 0) lido += r;
    }
    cout << "Valor recebido (int): " << valor << endl;
    closeSerial(fd);
  }

  void requestFloat() {
    int fd = request(Command::FLOAT_REQUEST), lido = 0;
    float valor = 0;
    while (lido < 4) {
      int r = read(fd, ((unsigned char *)&valor) + lido, 4 - lido);
      if (r > 0) lido += r;
    }
    cout << "Valor recebido (float): " << valor << endl;
    closeSerial(fd);
  }

  void requestString() {
    int fd = request(Command::STRING_REQUEST), lido = 0;
    uint8_t len;
    if (read(fd, &len, 1) != 1) throw runtime_error("Erro ao ler tamanho da string");
    string str(len, '\0');
    while (lido < len) {
      int r = read(fd, &str[lido], len - lido);
      if (r > 0) lido += r;
    }
    cout << "String recebida: " << str << endl;
    closeSerial(fd);
  }

private:
  string portName;
  speed_t baudRate;

  int openSerial() {
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

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      perror("Erro ao aplicar configurações na UART");
      close(fd);
      return -1;
    }

    return fd;
  }

  void closeSerial(int fd) {
    if (fd >= 0) close(fd);
  }
};

int main() {
  Uart uart("/dev/serial0", B9600);
  uart.requestInt();
  uart.requestFloat();
  uart.requestString();

  return 0;
}
