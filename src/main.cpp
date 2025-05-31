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

MotorController motorX(MOTOR_X_PWM, MOTOR_X_DIR1, MOTOR_X_DIR2,
                         ENCODER_X_A, ENCODER_X_B,
                         SENSOR_X_MIN, SENSOR_X_MAX,
                         300, 700);

MotorController motorY(MOTOR_Y_PWM, MOTOR_Y_DIR1, MOTOR_Y_DIR2,
                       ENCODER_Y_A, ENCODER_Y_B,
                       SENSOR_Y_MIN, SENSOR_Y_MAX,
                       300, 700);

void handler(int) {
  modbus.ensureClosed();
  bmp280.close();
  motorX.brake();
  motorY.brake();
  exit(0);
}

std::ostream &operator<<(ostream &os, const ModbusController::RegisterState &state) {
  os << "Movendo:    LEFT=" << state.isMoving[0]
     << " RIGHT=" << state.isMoving[1]
     << " UP=" << state.isMoving[2]
     << " DOWN=" << state.isMoving[3] << "\n";
  os << "Preset:     P1=" << state.readingPreset[0]
     << " P2=" << state.readingPreset[1]
     << " P3=" << state.readingPreset[2]
     << " P4=" << state.readingPreset[3]
     << " | Configurando?=" << state.isSettingPreset << "\n";
  os << "Calibrando?: " << state.isCalibrating << "\n";
  return os;
}

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
  modbus.write(speedRegister, data.speed);
  modbus.write(distanceRegister, data.distance);
}

void updateBMP280(){
  bmp280Data bmpData = bmp280.readData();
  modbus.write(ModbusController::SubCode::REG_TEMPERATURE, bmpData.temperature);
  modbus.write(ModbusController::SubCode::REG_PRESSURE, bmpData.pressure);
}

int main() {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);

  std::cout << "\033[2J\033[H";
  std::cout.flush();

  while (true) {
    auto screenState = modbus.readRegisters();

    std::cout << "\033[H\033[J";

    std::ostringstream output;
    output << screenState;

    std::cout << output.str();
    std::cout.flush();

    updateBMP280();
  }
  return 0;
}
