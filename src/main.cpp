#include <csignal>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <array>
#include <cmath>
#include <map>
#include <thread>

#include "modbusController.h"
#include "bmp280Controller.h"
#include "motorController.h"
#include "pidController.h"
#include "gpioController.h"

// ------------ MODBUS ------------

ModbusController modbus("/dev/serial0", B115200);

// ------------ BMP280 ------------

bmp280Controller bmp280;

void updateBMP280(){
  bmp280Data bmpData = bmp280.readData();
  modbus.write(ModbusController::SubCode::TEMP, bmpData.temperature);
  modbus.write(ModbusController::SubCode::PRESSURE, bmpData.pressure);
}

// ------------ MOTORS ------------

MotorController motorX(MOTOR_X_PWM,MOTOR_X_DIR1,MOTOR_X_DIR2,ENCODER_X_A,ENCODER_X_B,SENSOR_X_MIN,SENSOR_X_MAX,300,70);
MotorController motorY(MOTOR_Y_PWM,MOTOR_Y_DIR1,MOTOR_Y_DIR2,ENCODER_Y_A,ENCODER_Y_B,SENSOR_Y_MIN,SENSOR_Y_MAX,180,70);

void calibrate() {
  std::cout << "Calibrating..." << std::endl;
  modbus.init();
  modbus.write(ModbusController::SubCode::OP_STATE, std::byte{1});

  std::thread t1([] { motorX.calibrate(); });
  std::thread t2([] { motorY.calibrate(); });

  t1.join();
  t2.join();

  modbus.write(ModbusController::SubCode::OP_STATE, std::byte{0});
  std::cout << "Calibration done" << std::endl;
}

float lastPositionX = 0.0f, lastPositionY = 0.0f;

void updatePosition(MotorController &motor, bool isXMotor){
  const ModbusController::SubCode speedRegister = isXMotor ? ModbusController::SubCode::X_SPEED : ModbusController::SubCode::Y_SPEED;
  const ModbusController::SubCode distanceRegister = isXMotor ? ModbusController::SubCode::X_POS : ModbusController::SubCode::Y_POS;

  motorData data = motor.getMotorData();
  // arredonda pois 40% do tempo a velocidade fica 0.078 e mostra zero no dashboard
  modbus.write(speedRegister, (data.speed + 0.03f));
  modbus.write(distanceRegister, data.distance);

  if (isXMotor) lastPositionX = data.distance;
  else lastPositionY = data.distance;

#ifdef DEBUG
  std::cout << "Distance: " << data.distance << " m | speed: " << data.speed << " m/s" << std::endl;
#endif
}

void move(MotorController &motor, bool forward, bool isXMotor) {
  const bool limitReached = forward ? motor.onForwardLimit() : motor.onBackwardLimit();

  if (limitReached) {
    updatePosition(motor, isXMotor);
    return;
  }

  if (forward) motor.setForward();
  else motor.setBackward();

  usleep(50000);
  updatePosition(motor, isXMotor);
  motor.brake();
}

void moveToPosition(MotorController& motor, float targetPosition, bool isXMotor) {
  PIDController pid;
  pid.setReference(targetPosition);

  const float tolerance = 0.01f * fabs(targetPosition);

  while (true) {
    motorData data = motor.getMotorData();
    float currentPosition = data.distance;

    float error = targetPosition - currentPosition;
    if (fabs(error) <= tolerance) {
      motor.brake();
      break;
    }

    double controlSignal = pid.getControlSignal(currentPosition);
    float pwm = static_cast<float>(std::min(fabs(controlSignal), 100.0));

    if (controlSignal >= 0) motor.setForward(pwm);
    else motor.setBackward(pwm);

    usleep(50000);
    updatePosition(motor, isXMotor);
  }
  motor.brake();
  usleep(50000);
  updatePosition(motor, isXMotor);
}

// ------------ PID ------------

struct position {
  float x;
  float y;
};

array<position, 4> predefinedPositions = {};

void savePredefinedPosition(int position) {
  std::cout << "Preset: " << position << " done, Position: " << lastPositionX << ", " << lastPositionY << std::endl;
  predefinedPositions[position] = {lastPositionX, lastPositionY};
}

void goToPredefinedPosition(int position) {
  const auto&[x, y] = predefinedPositions[position];
  std::cout << "Preset: " << position << " moving to " << x << ", " << y << std::endl;
  if (x != 0 && y != 0) {
    std::thread t1([&] { moveToPosition(motorX, x, true); });
    std::thread t2([&] { moveToPosition(motorY, y, false); });
    t1.join();
    t2.join();
  }
  std::cout << "Preset: " << position << " done" << std::endl;
}

// ------------ GPIO ------------

GPIOController &gpio = GPIOController::getInstance();

void configurePins() {
  gpio.configureInputPin(BOTAO_CIMA);
  gpio.configureInputPin(BOTAO_BAIXO);
  gpio.configureInputPin(BOTAO_ESQ);
  gpio.configureInputPin(BOTAO_DIR);
}

// ------------ MOVIMENTAÇÃO ------------

void behavior(const ModbusController::RegisterState registers) {
  bool left   = gpio.getDigitalInput(BOTAO_ESQ) || registers.isMoving[0];
  bool right   = gpio.getDigitalInput(BOTAO_DIR) || registers.isMoving[1];
  bool up  = gpio.getDigitalInput(BOTAO_CIMA) || registers.isMoving[2];
  bool down = gpio.getDigitalInput(BOTAO_BAIXO) || registers.isMoving[3];

  int activeCount = up + down + left + right;
  if (activeCount > 1) return;

  if (up) move(motorY, true, false);
  if (down) move(motorY, false, false);
  if (left) move(motorX, false, true);
  if (right) move(motorX, true, true);
}

bool usingPreset = false;

void preset(const ModbusController::RegisterState registers) {
  if (registers.isSettingPreset) {
    std::cout << "Making Preset..." << std::endl;
    usingPreset = true;
    return;
  }
  if (registers.selectedPreset.has_value()) {
    if (usingPreset) {
      savePredefinedPosition(registers.selectedPreset.value());
      usingPreset = false;
      return;
    }
    goToPredefinedPosition(registers.selectedPreset.value());
  }
}

// ------------ MAIN ------------

void emergencyHandler() {
  modbus.init();
  modbus.ensureClosed();
  bmp280.close();
  std::cout << "Closing everything..." << std::endl;
  exit(0);
}

void signalHandler(int) {
  emergencyHandler();
}

int main() {
  signal(SIGINT, signalHandler);
  gpio.configureInterrupt(BOTAO_EMERGENCIA, emergencyHandler);
  configurePins();
  calibrate();

  while (true) {
    try {
      auto registers = modbus.readRegisters();
      updateBMP280();
      if (registers.isCalibrating) calibrate();
      preset(registers);
      behavior(registers);
    } catch (const std::exception &e) {
#ifdef DEBUG
      std::cerr << "Error: " << e.what() << "\n";
#endif
      continue;
    }
  }
  return 0;
}