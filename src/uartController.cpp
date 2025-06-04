#include "uartController.h"
#include "easylogging++.h"
#include <cstdint>
#include <fcntl.h>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <termios.h>
#include <unistd.h>

constexpr int UART_POLLING_INTERVAL = 50000; // 50 ms

using namespace std;

std::string toHexString(span<const uint8_t> data) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (const auto &byte : data) {
    oss << std::setw(2) << static_cast<int>(byte) << " ";
  }
  return oss.str();
}

void UARTController::ensureClosed() {
  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}

void UARTController::sync() {
  fsync(fd);
  usleep(UART_POLLING_INTERVAL);
}

void UARTController::send(span<const uint8_t> data) {
  LOG(INFO) << "Sending uart message: "
            << toHexString(data);
  int count = write(fd, data.data(), data.size());
  if (count < 0 || count != static_cast<int>(data.size()))
    throw std::system_error(errno, std::generic_category(),
                            "Erro ao escrever na porta serial");
  LOG(INFO) << "Message sent succesfully, " << count << " bytes";
}

void UARTController::send(const vector<uint8_t> &data) {
  send(span(data));
}

size_t UARTController::read_into(span<uint8_t> buffer) {
  ssize_t len = ::read(fd, buffer.data(), buffer.size());
  if (len < 0)
    throw std::system_error(errno, std::generic_category(),
                            "Erro ao ler da porta serial");
  LOG(INFO) << "Read UART Message: "
            << toHexString(span(buffer.data(), len));
  return len;
}

vector<uint8_t> UARTController::read(ssize_t max) {
  vector<uint8_t> buffer(max);
  size_t len = read_into(span(buffer));
  buffer.resize(len);
  return buffer;
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
