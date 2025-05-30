#include "motorController.h"
#include "gpioController.h"
#include <algorithm>
#include <thread>

MotorController::MotorController(int PWM, int DIR1, int DIR2, int ENCODER_A, int ENCODER_B, int MIN_SENSOR, int MAX_SENSOR, int trackLengthInCM)
    : PWM_OUT(PWM), DIR1(DIR1), DIR2(DIR2), ENCODER_A(ENCODER_A), ENCODER_B(ENCODER_B),
      MIN_SENSOR(MIN_SENSOR), MAX_SENSOR(MAX_SENSOR), trackLengthInCM(trackLengthInCM) {
  gpio.configurePWMPin(PWM_OUT);
  gpio.configureOutputPin(DIR1);
  gpio.configureOutputPin(DIR2);
  gpio.configureInputPin(ENCODER_A);
  gpio.configureInputPin(ENCODER_B);
  gpio.configureInputPin(MIN_SENSOR);
  gpio.configureInputPin(MAX_SENSOR);
}

void MotorController::init() {
  encoderThread = std::thread([this]() {
    while (true) {
      if (stopEncoder.load()) {
        break;
      }

      bool a = gpio.getDigitalInput(ENCODER_A);
      bool b = gpio.getDigitalInput(ENCODER_B);

      if (a && !b) {
        pulseCount.fetch_add(1);
      } else if (!a && b) {
        pulseCount.fetch_sub(1);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });
}

void MotorController::calibrate() {
  gpio.setDigitalOutput(DIR1, false);
  gpio.setDigitalOutput(DIR2, false);
  gpio.setPWMOutput(PWM_OUT, 0);
  pulseCount.store(0);
  speed = 0;

  while (gpio.getDigitalInput(MIN_SENSOR) != true) {
    setBackward(1023);
  }
  trackLengthInPulses = pulseCount.load();

  brake();
}

void MotorController::setFree() {
  gpio.setDigitalOutput(DIR1, false);
  gpio.setDigitalOutput(DIR2, false);
  gpio.setPWMOutput(PWM_OUT, 0);
  speed = 0;
}

void MotorController::setForward(int inputSpeed) {
  gpio.setDigitalOutput(DIR1, true);
  gpio.setDigitalOutput(DIR2, false);
  speed = std::clamp(inputSpeed, 0, 1023);
  gpio.setPWMOutput(PWM_OUT, speed);
}

void MotorController::setBackward(int inputSpeed) {
  gpio.setDigitalOutput(DIR1, false);
  gpio.setDigitalOutput(DIR2, true);
  speed = std::clamp(inputSpeed, 0, 1023);
  gpio.setPWMOutput(PWM_OUT, speed);
}

void MotorController::brake() {
  gpio.setDigitalOutput(DIR1, true);
  gpio.setDigitalOutput(DIR2, true);
  gpio.setPWMOutput(PWM_OUT, 0);
  speed = 0;
}

void MotorController::setSpeed(int inputSpeed) {
  speed = std::clamp(inputSpeed, 0, 1023);
  gpio.setPWMOutput(PWM_OUT, speed);
}

int MotorController::getSpeed() const {
  return speed;
}

long long int MotorController::getEncoderCount() const {
  return pulseCount.load();
}

void MotorController::resetEncoderCount() {
  pulseCount.store(0);
}

MotorController::~MotorController() {
  stopEncoder.store(true);
  if (encoderThread.joinable()) {
    encoderThread.join();
  }
  brake();
}
