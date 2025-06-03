#include <csignal>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <array>
#include <cmath>
#include <map>
#include <thread>
#include <atomic>

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
MotorController motorY(MOTOR_Y_PWM,MOTOR_Y_DIR1,MOTOR_Y_DIR2,ENCODER_Y_A,ENCODER_Y_B,SENSOR_Y_MIN,SENSOR_Y_MAX,300,70);

void calibrate() {
  modbus.write(ModbusController::SubCode::REG_MACHINE_STATE, 0x01);

  std::thread t1([] { motorX.calibrate(); });
  std::thread t2([] { motorY.calibrate(); });

  t1.join();
  t2.join();

  modbus.write(ModbusController::SubCode::REG_MACHINE_STATE, 0x00);
}

float ultimaPosicaoX = 0.0f, ultimaPosicaoY = 0.0f;

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
  modbus.write(speedRegister, data.speed);
  modbus.write(distanceRegister, data.distance);

  if (isXMotor) ultimaPosicaoX = data.distance;
  else ultimaPosicaoY = data.distance;
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

  modbus.write(speedRegister, data.speed);
  modbus.write(distanceRegister, data.distance);
}

// ------------ PID ------------

struct position {
  float x;
  float y;
};

array<position, 4> predefinedPositions = {};

void savePredefinedPosition(int position) {
  predefinedPositions[position] = {motorX.getMotorData().distance, motorY.getMotorData().distance};
}

void goToPredefinedPosition(int position) {
  if (predefinedPositions[position].x != 0 && predefinedPositions[position].y != 0) {
    moveToPosition(motorX, predefinedPositions[position].x, true);
    moveToPosition(motorY, predefinedPositions[position].y, false);
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

struct botao {
  int pino;
  std::string nome;
  bool estado;
};

// ------------ MAIN ------------

void emergencyHandler() {
  modbus.ensureClosed();
  bmp280.close();
  moveToPosition(motorX, ultimaPosicaoX, true);
  moveToPosition(motorY, ultimaPosicaoY, false);
  std::cout << "Botão de emergência acionado! Fechando tudo..." << std::endl;
  exit(0);
}

void signalHandler(int) {
  emergencyHandler();
}

int main() {
  signal(SIGINT, signalHandler);
  gpio.configureInterrupt(BOTAO_EMERGENCIA, emergencyHandler);
  calibrate();

  std::array<botao, 4> botoes = {
    botao{BOTAO_CIMA, "Cima", false},
    botao{BOTAO_BAIXO, "Baixo", false},
    botao{BOTAO_ESQ, "Esquerda", false},
    botao{BOTAO_DIR, "Direita", false}};

  bool botaoPressionado = false;
  int pinoBotaoPressionado = -1;

  while (true) {
    botaoPressionado = false;
    for (auto &botao : botoes) {
      bool estadoAtual = digitalRead(botao.pino);

      if (estadoAtual != botao.estado) {
        botao.estado = estadoAtual;

        if (estadoAtual) {
          std::cout << "Botão " << botao.nome << " pressionado!" << std::endl;
          pinoBotaoPressionado = botao.pino;
        }
        else {
          std::cout << "Botão " << botao.nome << " liberado!" << std::endl;

          if (botao.pino == BOTAO_CIMA || botao.pino == BOTAO_BAIXO)
            moveToPosition(motorY, ultimaPosicaoY, false);
          else if (botao.pino == BOTAO_DIR || botao.pino == BOTAO_ESQ)
            moveToPosition(motorX, ultimaPosicaoX, true);

          if (botao.pino == pinoBotaoPressionado)
            pinoBotaoPressionado = -1;
        }
      }
      if (estadoAtual) {
        botaoPressionado = true;
        pinoBotaoPressionado = botao.pino;
      }
    }

    if (botaoPressionado) {
      if (pinoBotaoPressionado == BOTAO_CIMA)
        move(motorY, true, false);
      else if (pinoBotaoPressionado == BOTAO_BAIXO)
        move(motorY, false, false);
      else if (pinoBotaoPressionado == BOTAO_DIR)
        move(motorX, true, true);
      else if (pinoBotaoPressionado == BOTAO_ESQ)
        move(motorX, false, true);
    }
    delay(50);
  }
  return 0;
}

