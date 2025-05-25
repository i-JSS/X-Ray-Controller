#pragma once
#include "uartController.h"
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace std;

class UARTController {
public:
  UARTController(const string &portName, const speed_t baudRate)
      : portName(portName), baudRate(baudRate) {}
  ~UARTController() { ensureClosed(); }

  void ensureOpen();
  void ensureClosed();

  void send(const span<uint8_t> data);
  void read_into(span<uint8_t> buffer);
  vector<uint8_t> read(ssize_t max);

private:
  int fd;
  string portName;
  speed_t baudRate;
};
