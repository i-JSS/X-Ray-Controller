#include "gpioController.h"
#include "motorController.h"
#include <array>
#include <iostream>
#include <unistd.h>

void quit() {
  // Stop all motors and quit
  exit(0);
}

bool move(MotorController &motor, bool forward) {
  const bool limitReached = forward ? motor.onForwardLimit() : motor.onBackwardLimit();

  if (limitReached) return true;


  if (forward) motor.setForward();
  else motor.setBackward();

  usleep(50000);
  motorData data = motor.getMotorData();
  motor.free();
  std::cout << "Distância: " << data.distance << " m | Velocidade: " << data.speed << " m/s" << std::endl;
  return false;
}


int main(void) {
  MotorController motorX(MOTOR_X_PWM, MOTOR_X_DIR1, MOTOR_X_DIR2,
                         ENCODER_X_A, ENCODER_X_B,
                         SENSOR_X_MIN, SENSOR_X_MAX,
                         300, 70);

  MotorController motorY(MOTOR_Y_PWM, MOTOR_Y_DIR1, MOTOR_Y_DIR2,
                         ENCODER_Y_A, ENCODER_Y_B,
                         SENSOR_Y_MIN, SENSOR_Y_MAX,
                         300, 70);
  std::cout << "Movendo para frente...\n";
  while (true) {
    if (move(motorX, true)) break;
  }

  std::cout << "Movendo para trás...\n";
  while (true) {
    if (move(motorX, false)) break;
  }

  return 0;
}

