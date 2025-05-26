#ifndef I2CCONTROLLER_H
#define I2CCONTROLLER_H

#include "bme280.h"
#include <string>
#include <cstring>
#include <cstdint>

class i2cController {
public:
    i2cController(const std::string& i2cDevice, const uint8_t address)
        : fd(-1), devicePath(i2cDevice), i2cAddress(address) {}

    void ensureOpen();
    void ensureClose();
    bool read(int8_t address, char *data, int size, int num1, int num2);
    bool write(uint8_t reg, uint8_t value);

private:
    int fd;
    std::string devicePath;
    uint8_t i2cAddress;
};

#endif // I2CCONTROLLER_H
