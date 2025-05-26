#include "bmp280Controller.h"
#include <array>
#include <iostream>
#include <unistd.h>

using namespace std;

bmp280Controller::bmp280Controller() : i2c("/dev/i2c-1", 0x76) {
    i2c.ensureOpen();
    configure();
    converter = coefficient();
    i2c.ensureClose();
}

void bmp280Controller::initialize() {
    i2c.ensureOpen();
}

void bmp280Controller::close(){
    i2c.ensureClose();
}

void bmp280Controller::configure() {
    if (!i2c.write(0xF4, 0x27)) {
        std::cerr << "Erro configurando ctrl_meas" << std::endl;
        return;
    }

    if (!i2c.write(0xF5, 0xA0)) {
        std::cerr << "Erro configurando config" << std::endl;
        return;
    }
    sleep(1);
}

array<int, 12> bmp280Controller::coefficient() {
    char data[24] = {0};

    if (!i2c.read(0x88, data, 24, 1, 1)) {
        std::cout << "Failed to read I2C device" << std::endl;
        return {};
    }

    array<int, 12> result = {};
    int index = 0;

    for (int i = 0; i < 24; i+=2){
       int value = data[i+1] * 256 + data[i];
       if (i != 0 && i != 6) {
          if(value > 32767) value -= 65536;
       }
       result[index++] = value;
    }
    return result;
}

float bmp280Controller::temperature(char *data) {
    long adc_t = (((long)data[3] * 65536) + ((long)data[4] * 256) + (long)(data[5] & 0xF0)) / 16;

    double var1 = (((double)adc_t) / 16384.0 - ((double)converter[0]) / 1024.0) * ((double)converter[1]);
    double var2 = ((((double)adc_t) / 131072.0 - ((double)converter[0]) / 8192.0) *
                  (((double)adc_t)/131072.0 - ((double)converter[0])/8192.0)) * ((double)converter[2]);

    t_fine = (long)(var1 + var2);
    return (float)((var1 + var2) / 5120.0);
}

float bmp280Controller::pressure(char *data) {
    long adc_p = (((long)data[0] * 65536) + ((long)data[1] * 256) + (long)(data[2] & 0xF0)) / 16;

    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * ((double)converter[8]) / 32768.0;
    var2 = var2 + var1 * ((double)converter[7]) * 2.0;
    var2 = (var2 / 4.0) + (((double)converter[6]) * 65536.0);
    var1 = (((double)converter[5]) * var1 * var1 / 524288.0 + ((double)converter[4]) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)converter[3]);

    double p = 1048576.0 - (double)adc_p;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)converter[11]) * p * p / 2147483648.0;
    var2 = p * ((double)converter[10]) / 32768.0;
    return (float)((p + (var1 + var2 + ((double)converter[9])) / 16.0) / 100);
}

array<float, 2> bmp280Controller::readData() {
    char data[8] = {0};
    if (!i2c.read(0xF7, data, 8, 0, 1)) {
        std::cout << "Failed to read I2C device" << std::endl;
        return {};
    }

    float temp = temperature(data);
    float press = pressure(data);

    std::cout << "Temperature: " << temp << " °C" << endl;
    std::cout << "Pressure: " << press << " hPa" << endl;

    return {temp, press};
}
