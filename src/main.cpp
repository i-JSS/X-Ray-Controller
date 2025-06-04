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
  modbus.write(ModbusController::SubCode::REG_TEMPERATURE, bmpData.temperature);
  modbus.write(ModbusController::SubCode::REG_PRESSURE, bmpData.pressure);
}

// ------------ MOTORS ------------

MotorController motorX(MOTOR_X_PWM,MOTOR_X_DIR1,MOTOR_X_DIR2,ENCODER_X_A,ENCODER_X_B,SENSOR_X_MIN,SENSOR_X_MAX,300,70);
MotorController motorY(MOTOR_Y_PWM,MOTOR_Y_DIR1,MOTOR_Y_DIR2,ENCODER_Y_A,ENCODER_Y_B,SENSOR_Y_MIN,SENSOR_Y_MAX,180,70);

void calibrate() {
  modbus.write(ModbusController::SubCode::REG_MACHINE_STATE, 0x01);

  std::thread t1([] { motorX.calibrate(); });
  std::thread t2([] { motorY.calibrate(); });

  t1.join();
  t2.join();

  modbus.write(ModbusController::SubCode::REG_MACHINE_STATE, 0x00);
}

float lastPositionX = 0.0f, lastPositionY = 0.0f;

void move(MotorController &motor, bool forward, bool isXMotor) {
  const bool limitReached = forward ? motor.onForwardLimit() : motor.onBackwardLimit();
  const ModbusController::SubCode speedRegister = isXMotor ? ModbusController::SubCode::REG_SPEED_X : ModbusController::SubCode::REG_SPEED_Y;
  const ModbusController::SubCode distanceRegister = isXMotor ? ModbusController::SubCode::REG_POSITION_X : ModbusController::SubCode::REG_POSITION_Y;
  // ADICIONAR A REDUÇÃO DE VELOCIDADE AQUI (ESPERAR O FREIO FUNCIONAR)
  if (limitReached) return;

  if (forward) motor.setForward();
  else motor.setBackward();

  usleep(50000);
  motorData data = motor.getMotorData();
  motor.brake();
  std::cout << "Distância: " << data.distance << " m | Velocidade: " << data.speed << " m/s" << std::endl;
  // arredonda pois 40% do tempo a velocidade fica 0.078 e mostra zero no dashboard
  modbus.write(speedRegister, (data.speed + 0.03f));
  modbus.write(distanceRegister, data.distance);

  if (isXMotor) lastPositionX = data.distance;
  else lastPositionY = data.distance;
}

void moveToPosition(MotorController& motor, float targetPosition, bool isXMotor) {
  PIDController pid;
  pid.setReference(targetPosition);

  const float tolerance = 0.01f * fabs(targetPosition);

  const ModbusController::SubCode speedRegister = isXMotor ? ModbusController::SubCode::REG_SPEED_X : ModbusController::SubCode::REG_SPEED_Y;
  const ModbusController::SubCode distanceRegister = isXMotor ? ModbusController::SubCode::REG_POSITION_X : ModbusController::SubCode::REG_POSITION_Y;

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
  }
  motor.brake();
  motorData data = motor.getMotorData();

#ifdef DEBUG
  std::cout << "Distância: " << data.distance << " m | Velocidade: " << data.speed << " m/s" << std::endl;
#endif

  modbus.write(speedRegister, (data.speed + 0.03f));
  modbus.write(distanceRegister, data.distance);
}

// ------------ PID ------------

struct position {
  float x;
  float y;
};

array<position, 4> predefinedPositions = {};

void savePredefinedPosition(int position) {
  predefinedPositions[position] = {lastPositionX, lastPositionY};
}

void goToPredefinedPosition(int position) {
  const auto&[x, y] = predefinedPositions[position];
  if (x != 0 && y != 0) {
    std::thread t1([&] { moveToPosition(motorX, x, true); });
    std::thread t2([&] { moveToPosition(motorY, y, false); });
    t1.join();
    t2.join();
  }
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
  bool up  = gpio.getDigitalInput(BOTAO_CIMA) || registers.isMoving[0];
  bool down = gpio.getDigitalInput(BOTAO_BAIXO) || registers.isMoving[1];
  bool left   = gpio.getDigitalInput(BOTAO_ESQ) || registers.isMoving[2];
  bool right   = gpio.getDigitalInput(BOTAO_DIR) || registers.isMoving[3];

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
    usingPreset = true;
    return;
  }
  if (usingPreset) {
    for (int i = 0; i < 4; ++i)
      if (registers.readingPreset[i])
        savePredefinedPosition(i);
        usingPreset = false;
        return;
  }
  for (int i = 0; i < 4; ++i)
    if (registers.readingPreset[i])
      goToPredefinedPosition(i);
}

// ------------ MAIN ------------

void emergencyHandler() {
  // LIMPART UART
  // modbus.init();
  modbus.ensureClosed();
  bmp280.close();
  // moveToPosition(motorX, ultimaPosicaoX, true);
  // moveToPosition(motorY, ultimaPosicaoY, false);
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
  // modbus.init();
  updateBMP280();
  calibrate();
  while (true) {
    updateBMP280();
    auto registers = modbus.readRegisters();
    if (registers.isCalibrating) calibrate();
    preset(registers);
    behavior(registers);
  }
  return 0;
}