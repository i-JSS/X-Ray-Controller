#ifndef I2CCONTROLLER_H
#define I2CCONTROLLER_H

#include "bme280.h"
#include <string>
#include <cstring>
#include <cstdint>

class i2cController {
public:
    i2cController(const std::string& i2c_device, const uint8_t address)
        : fd(-1), device_path(i2c_device), i2c_address(address) {
        std::memset(&dev, 0, sizeof(dev));
    }

    void ensureOpen();
    void ensureClose();

    int8_t readReg(uint8_t reg_addr, uint8_t *data, uint32_t len);
    int8_t writeReg(uint8_t reg_addr, const uint8_t *data, uint32_t len);

    bme280_dev dev;

private:
    int fd;
    std::string device_path;
    uint8_t i2c_address;
};

#endif // I2CCONTROLLER_H
