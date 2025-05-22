#include <cstdint>
#include <fcntl.h>
#include <span>
#include <system_error>
#include <termios.h>
#include <unistd.h>
#define SERIAL_PORT "/dev/serial0"

class UARTController {
  int uart0_filestream = -1;

  UARTController() {
    uart0_filestream = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart0_filestream == -1) {
      throw std::system_error(errno, std::generic_category(),
                              "Failed to open " SERIAL_PORT);
    }

    termios options;
    tcgetattr(uart0_filestream, &options);

    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);
  }

  void send(std::span<uint8_t> data) {
    int count = write(uart0_filestream, data.data(), data.size());
    if (count <= 0) {
      throw std::system_error(errno, std::generic_category(),
                              "Failed to open " SERIAL_PORT);
    }
  }

  ~UARTController() { close(uart0_filestream); }
};
