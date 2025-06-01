#include "bmp280Controller.h"
#include "modbusController.h"
#include <csignal>
#include <iostream>
#include <ostream>
#include <sstream>
#include <unistd.h>
#include "motorController.h"
#include "gpioController.h"

ModbusController modbus("/dev/serial0", B115200);
bmp280Controller bmp280;
GPIOController &gpio = GPIOController::getInstance();

MotorController motorX(MOTOR_X_PWM, MOTOR_X_DIR1, MOTOR_X_DIR2,
                         ENCODER_X_A, ENCODER_X_B,
                         SENSOR_X_MIN, SENSOR_X_MAX,
                         300, 70);

MotorController motorY(MOTOR_Y_PWM, MOTOR_Y_DIR1, MOTOR_Y_DIR2,
                       ENCODER_Y_A, ENCODER_Y_B,
                       SENSOR_Y_MIN, SENSOR_Y_MAX,
                       300, 70);

void handler(int) {
  modbus.ensureClosed();
  bmp280.close();
  motorX.brake();
  motorY.brake();
  std::cout << "Closing UART, I2C, STOP MOTORS" << std::endl;
  exit(0);
}

// std::ostream &operator<<(ostream &os, const ModbusController::RegisterState &state) {
//   os << "Movendo:    LEFT=" << state.isMoving[0]
//      << " RIGHT=" << state.isMoving[1]
//      << " UP=" << state.isMoving[2]
//      << " DOWN=" << state.isMoving[3] << "\n";
//   os << "Preset:     P1=" << state.readingPreset[0]
//      << " P2=" << state.readingPreset[1]
//      << " P3=" << state.readingPreset[2]
//      << " P4=" << state.readingPreset[3]
//      << " | Configurando?=" << state.isSettingPreset << "\n";
//   os << "Calibrando?: " << state.isCalibrating << "\n";
//   return os;
// }

void move(MotorController &motor, bool forward, bool isXMotor) {
  const bool limitReached = forward ? motor.onForwardLimit() : motor.onBackwardLimit();
  const ModbusController::SubCode speedRegister = isXMotor ? ModbusController::SubCode::REG_SPEED_X : ModbusController::SubCode::REG_SPEED_Y;
  const ModbusController::SubCode distanceRegister = isXMotor ? ModbusController::SubCode::REG_POSITION_X : ModbusController::SubCode::REG_POSITION_Y;
  if (limitReached) return;

  if (forward) motor.setForward();
  else motor.setBackward();

  usleep(50000);
  motorData data = motor.getMotorData();
  motor.brake();
  std::cout << "Distância: " << data.distance << " m | Velocidade: " << data.speed << " m/s" << std::endl;
  // modbus.write(speedRegister, data.speed);
  // modbus.write(distanceRegister, data.distance);
}

void updateBMP280(){
  bmp280Data bmpData = bmp280.readData();
  modbus.write(ModbusController::SubCode::REG_TEMPERATURE, bmpData.temperature);
  modbus.write(ModbusController::SubCode::REG_PRESSURE, bmpData.pressure);
}

void configurePins() {
  gpio.configureInputPin(BOTAO_CIMA);
  gpio.configureInputPin(BOTAO_BAIXO);
  gpio.configureInputPin(BOTAO_ESQ);
  gpio.configureInputPin(BOTAO_DIR);
}

int lastButtonX;
int lastButtonY;

// void removeMutualExclusion() {
//   int inverseX = (lastButtonX == BOTAO_ESQ) ? BOTAO_DIR : BOTAO_ESQ;
//   if (gpio.getDigitalInput(lastButtonX) && gpio.getDigitalInput(inverseX)) {
//     gpio.setDigitalOutput(lastButtonX, false);
//     if (lastButtonX == BOTAO_ESQ)
//       move(motorX, false, true);
//     else
//       move(motorX, false, true);
//   }
//
//   int inverseY = (lastButtonY == BOTAO_CIMA) ? BOTAO_BAIXO : BOTAO_CIMA;
//   if (gpio.getDigitalInput(lastButtonY) && gpio.getDigitalInput(inverseY))
//     gpio.setDigitalOutput(lastButtonY, false);
// }

void checkValues() {
  if (gpio.getDigitalInput(BOTAO_CIMA)) {
    lastButtonY = BOTAO_CIMA;
    move(motorY, true, false);
  }
  if (gpio.getDigitalInput(BOTAO_BAIXO)) {
    lastButtonY = BOTAO_BAIXO;
    move(motorY, false, false);
  }
  if (gpio.getDigitalInput(BOTAO_ESQ)) {
    lastButtonX = BOTAO_ESQ;
    move(motorX, false, true);
  }
  if (gpio.getDigitalInput(BOTAO_DIR)) {
    lastButtonX = BOTAO_DIR;
    move(motorX, true, true);
  }
  // removeMutualExclusion();
}

void calibrate() {
  // modbus.write(ModbusController::SubCode::REG_MACHINE_STATE, 0x01);
  motorX.calibrate();
  motorY.calibrate();
  // modbus.write(ModbusController::SubCode::REG_MACHINE_STATE, 0x00);
}

int main() {
  signal(SIGINT, handler);
  configurePins();
  calibrate();
  while (true) {
    // auto screenState = modbus.readRegisters();
    checkValues();
    // updateBMP280();
  }
  return 0;
}
