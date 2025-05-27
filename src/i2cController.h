#pragma once
#include <cstdint>
#include <cstring>
#include <string>

class i2cController {
public:
  i2cController(const std::string &i2cDevice, const uint8_t address)
      : fd(-1), devicePath(i2cDevice), i2cAddress(address) {}

  void ensureOpen();
  void ensureClose();
  void read(int8_t address, char *data, int size, int n);
  void write(char *data, int size);

private:
  int fd;
  std::string devicePath;
  uint8_t i2cAddress;
};
