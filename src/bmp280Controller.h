#ifndef BMP280CONTROLLER_H
#define BMP280CONTROLLER_H

#include "i2cController.h"
#include <array>

using namespace std;

struct bmp280Data {
	float temperature;
	float pressure;
};

class bmp280Controller {
public:
    bmp280Controller();

    void initialize();
    void close();
    bmp280Data readData();

private:
    i2cController i2c;
    array<int, 12> converter;
    long t_fine;
    void configure();
    array<int, 12> coefficient();
    float getTemperature(char *data);
    float getPressure(char *data);
};

#endif //BMP280CONTROLLER_H