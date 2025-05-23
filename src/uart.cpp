#include "uart.h"
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <system_error>
#include <termios.h>
#include <unistd.h>

UARTController::UARTController() {
  uart0_filestream = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
  if (uart0_filestream == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to open " SERIAL_PORT);
  }

  termios options{
      .c_cflag = B115200 | CS8 | CLOCAL | CREAD,
      .c_iflag = IGNPAR,
      .c_oflag = 0,
      .c_lflag = 0,
  };
  tcgetattr(uart0_filestream, &options);

  tcflush(uart0_filestream, TCIFLUSH);
  tcsetattr(uart0_filestream, TCSANOW, &options);
}

void UARTController::send(std::span<uint8_t> data) {
  int count = write(uart0_filestream, data.data(), data.size());
  if (count < 0)
    throw std::system_error(errno, std::generic_category(),
                            "Failed to write data");
}

void UARTController::read_into(std::span<uint8_t> buffer) {
  int count = read(uart0_filestream, buffer.data(), buffer.size());
  if (count < 0)
    throw std::system_error(errno, std::generic_category(),
                            "Failed to read data");
}

UARTController::~UARTController() { close(uart0_filestream); }
