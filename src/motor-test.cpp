#include "gpioController.h"
#include "motorController.h"
#include <array>
#include <iostream>
#include <unistd.h>

void quit() {
  // Stop all motors and quit
  exit(0);
}

int main(void) {

  MotorController motor1(MOTOR_X_PWM, MOTOR_X_DIR1, MOTOR_X_DIR2, ENCODER_X_A, ENCODER_X_B, SENSOR_X_MIN, SENSOR_X_MAX, 300);
  while (true) {
    motor1.setBackward(100);
    sleep(5);

    motor1.brake();
  }
  return 0;
}
