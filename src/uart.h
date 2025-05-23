#pragma once
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <system_error>
#include <termios.h>
#include <unistd.h>
#define SERIAL_PORT "/dev/serial0"

class UARTController {
  int uart0_filestream = -1;

public:
  UARTController();

  void send(std::span<uint8_t> data);

  void read_into(std::span<uint8_t> buffer);

  ~UARTController();
};
