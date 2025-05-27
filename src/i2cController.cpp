#include "i2cController.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <iostream>

void i2cController::read(int8_t address, char *data, int size, int n) {
    char reg[n];
    reg[0] = address;
	write(reg, n);
    if (::read(fd, data, size) != size)
		throw std::runtime_error("Failed to read from i2c device");
}

void i2cController::write(char *data, int size) {
    if (::write(fd, data, size) != size)
		throw std::runtime_error("Failed to write to i2c device");
}

void i2cController::ensureOpen() {
    fd = open(devicePath.c_str(), O_RDWR);
    if (fd < 0) {
		close(fd);
		throw std::runtime_error("Failed to open i2c device");
    }
    if (ioctl(fd, I2C_SLAVE, i2cAddress) < 0) {
		close(fd);
		throw std::runtime_error("Failed to config I2C_SLAVE");
    }
}

void i2cController::ensureClose() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}