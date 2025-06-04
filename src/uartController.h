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

constexpr int UART_POLLING_INTERVAL = 50000; // 50 ms
constexpr int DEBUG_N = 40;                  // Frequência de log
//
using namespace std;

class UARTController {
public:
  UARTController(const string &portName, const speed_t baudRate)
      : portName(portName), baudRate(baudRate) {}
  ~UARTController() { ensureClosed(); }

  void ensureOpen();
  void ensureClosed();

  void send(span<const uint8_t> data);
  void send(const vector<uint8_t> &data);
  size_t read_into(span<uint8_t> buffer);
  vector<uint8_t> read(ssize_t max);

  void sync();

private:
  int fd;
  string portName;
  speed_t baudRate;
};
