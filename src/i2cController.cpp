#include "i2cController.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <iostream>

bool read(int8_t address, char *data, int size, int num1, int num2) {
    char reg[num1] = {address};
    write(fd, reg, num2);
    if (read(fd, data, size) != size){
        std::cerr << "Falha ao ler o endereço: " << address << std::endl;
        return false;
    }
    return true;
}

bool i2cController::write(uint8_t reg, uint8_t value) {
    char buffer[2] = { (char)reg, (char)value };
    if (write(fd, buffer, 2) != 2) {
        std::cerr << "Erro ao escrever no registrador 0x" << std::hex << (int)reg << std::dec << std::endl;
        return false;
    }
    return true;
}

void i2cController::ensureOpen() {
    fd = open(devicePath.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Erro abrindo I2C: " << devicePath << "\n";
        close(fd);
    }

    if (ioctl(fd, I2C_SLAVE, i2cAddress) < 0) {
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