#ifndef BMP280CONTROLLER_H
#define BMP280CONTROLLER_H

#include "i2cController.h"
#include <array>

using namespace std;

class bmp280Controller {
public:
    bmp280Controller();

    void initialize();
    void close();
    array<float, 2> readData();

private:
    i2cController i2c;
    array<int, 12> converter;
    long t_fine;
    void configure();
    array<int, 12> coefficient();
    float temperature(char *data);
    float pressure(char *data);
};

#endif //BMP280CONTROLLER_H