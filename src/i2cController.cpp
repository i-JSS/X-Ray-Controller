#include "i2cController.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <iostream>

int8_t i2cController::readReg(const uint8_t reg_addr, uint8_t *data, const uint32_t len) {
    if (write(fd, &reg_addr, 1) != 1)
        return BME280_E_COMM_FAIL;
    if (read(fd, data, len) != static_cast<int>(len))
        return BME280_E_COMM_FAIL;
    return BME280_OK;
}

int8_t i2cController::writeReg(const uint8_t reg_addr, const uint8_t *data, const uint32_t len) {
    uint8_t buffer[len + 1];
    buffer[0] = reg_addr;
    std::memcpy(buffer + 1, data, len);
    if (write(fd, buffer, len + 1) != static_cast<int>(len + 1))
        return BME280_E_COMM_FAIL;
    return BME280_OK;
}

void i2cController::ensureOpen() {
    fd = open(device_path.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Erro abrindo I2C: " << device_path << "\n";
        close(fd);
    }

    if (ioctl(fd, I2C_SLAVE, i2c_address) < 0) {
        std::cerr << "Erro configurando I2C_SLAVE\n";
        close(fd);
    }
}

void i2cController::ensureClose() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}