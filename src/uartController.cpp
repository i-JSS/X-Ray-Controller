#include "uartController.h"
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <termios.h>
#include <unistd.h>

using namespace std;

void UARTController::ensureClosed() {
  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}

void UARTController::send(const span<uint8_t> data) {
  int count = write(fd, data.data(), data.size());
  if (count < 0 || count != static_cast<int>(data.size()))
    throw std::system_error(errno, std::generic_category(),
                            "Falha ao escrever dados");
  fsync(fd);
  usleep(100000);
}

void UARTController::read_into(span<uint8_t> buffer) {
  ssize_t bytesRead;
  while (bytesRead < buffer.size()) {
    ssize_t r =
        read(fd, buffer.subspan(bytesRead).data(), buffer.size() - bytesRead);
    if (r > 0)
      bytesRead += r;
    // NOTE: não sei se é pra pra adicionar esse error handling aqui
    // else if (r == 0)
    //   break; // EOF ou sem dados
    // else
    //   throw std::system_error(errno, std::generic_category(),
    //                           "Erro ao ler dados da UART");
  }
}

void UARTController::ensureOpen() {
  fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd == -1)
    throw std::system_error(errno, std::generic_category(),
                            "Erro ao abrir a porta serial");

  termios tty{};
  if (tcgetattr(fd, &tty) != 0) {
    close(fd);
    throw std::system_error(errno, std::generic_category(),
                            "Erro ao obter atributos da UART");
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
    close(fd);
    throw std::system_error(errno, std::generic_category(),
                            "Erro ao aplicar configurações na UART");
  }
}
