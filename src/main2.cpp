#include "i2cController.h"
#include "bme280Sensor.h"
#include <iostream>

int main() {
    i2cController controller("/dev/i2c-1", 0x76);
    bme280Sensor sensor(&controller);
    sensor.initializeSensor();
    sensor.readSensorData();
    return 0;
}

