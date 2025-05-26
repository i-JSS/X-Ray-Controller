#include "bme280Sensor.h"
#include <iostream>
#include <unistd.h>


// API em C precisa dessas funções para conseguir acessar a api dentro da classe.

int8_t i2c_read(uint8_t reg_addr, uint8_t *data,  uint32_t len, void *intf_ptr) {
    return static_cast<i2cController*>(intf_ptr)->readReg(reg_addr, data, len);
}

int8_t i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr) {
    return static_cast<i2cController*>(intf_ptr)->writeReg(reg_addr, data, len);
}

void delay_us(const uint32_t period, void *intf_ptr) {
    (void)intf_ptr;
    usleep(period);
}

bme280Sensor::bme280Sensor(i2cController* controller) : i2c(controller) {
    dev.intf = BME280_I2C_INTF;
    dev.read = i2c_read;
    dev.write = i2c_write;
    dev.delay_us = delay_us;
    dev.intf_ptr = controller;
}

void bme280Sensor::initializeSensor() {
    i2c->ensureOpen();
    if (bme280_init(&dev) != BME280_OK) {
        std::cerr << "Erro inicializando BME280\n";
        i2c->ensureClose();
    }

    bme280_settings settings = {};
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_16X;
    settings.osr_t = BME280_OVERSAMPLING_2X;
    settings.filter = BME280_FILTER_COEFF_4;
    settings.standby_time = BME280_STANDBY_TIME_1000_MS;

    constexpr uint8_t settings_sel = BME280_SEL_OSR_PRESS |
                                     BME280_SEL_OSR_TEMP |
                                     BME280_SEL_OSR_HUM |
                                     BME280_SEL_FILTER |
                                     BME280_SEL_STANDBY;

    if (bme280_set_sensor_settings(settings_sel, &settings, &dev) != BME280_OK) {
        std::cerr << "Erro configurando sensor\n";
        i2c->ensureClose();
    }

    if (bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev) != BME280_OK) {
        std::cerr << "Erro definindo modo normal\n";
        i2c->ensureClose();
    }

}

void bme280Sensor::readSensorData() {
    initializeSensor();

    bme280_data comp_data{};
    int8_t rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &dev);
    if (rslt != BME280_OK) {
        std::cerr << "Erro lendo dados do sensor\n";
        i2c->ensureClose();
    }

    std::cout << comp_data.temperature << comp_data.pressure << comp_data.humidity << std::endl;

    i2c->ensureClose();
}
