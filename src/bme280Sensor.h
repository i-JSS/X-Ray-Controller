#ifndef BME280SENSOR_H
#define BME280SENSOR_H

#include "i2cController.h"
#include "bme280.h"

class bme280Sensor {
public:
    explicit bme280Sensor(i2cController* controller);

    void initializeSensor();
    void readSensorData();

private:
    bme280_dev dev{};
    i2cController* i2c;
};

int8_t i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr);
int8_t i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr);
void delay_us(const uint32_t period, void *intf_ptr);

#endif // BME280SENSOR_H
