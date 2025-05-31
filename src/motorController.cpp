#include "motorController.h"
#include "gpioController.h"
#include <algorithm>
#include <thread>
#include <unistd.h>
#include <iostream>

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

void MotorController::updateEncoder() {
  bool a = gpio.getDigitalInput(ENCODER_A);
  bool b = gpio.getDigitalInput(ENCODER_B);

  if (a && !prevA) {
    if (b) pulseCount.fetch_sub(1);
    else pulseCount.fetch_add(1);
  }

  prevA = a;
  prevB = b;
}

void MotorController::calibrate() {
  constexpr int CALIBRATION_SPEED = 300;
  constexpr int DIST_FROM_LIMIT_CM = 3;

  setBackward(CALIBRATION_SPEED);
  while (!gpio.getDigitalInput(MIN_SENSOR)) {
    updateEncoder();
    usleep(10000);
  }
  std::cout << "Minimum limit" << std::endl;
  brake();
  resetEncoderCount();

  setForward(CALIBRATION_SPEED);
  while (!gpio.getDigitalInput(MAX_SENSOR)) {
    updateEncoder();
    usleep(10000);
  }
  std::cout << "Maximum limit" << std::endl;
  brake();

  trackLengthInPulses = pulseCount.load();
  std::cout << "Total pulses counted: " << trackLengthInPulses << std::endl;
  std::cout << "length (cm): " << trackLengthInCM << std::endl;

  if (trackLengthInPulses == 0 || trackLengthInCM == 0)
    std::cerr << "Calibration error" << std::endl;

  const double cmPerPulse = static_cast<double>(trackLengthInCM) / trackLengthInPulses;
  const long long int marginPulses = static_cast<long long int>(DIST_FROM_LIMIT_CM / cmPerPulse);

  virtualMinLimit = marginPulses;
  virtualMaxLimit = trackLengthInPulses - marginPulses;


  std::cout << "Minimum virtual limit set at: " << virtualMinLimit << " pulses" << std::endl;
  std::cout << "Maximum virtual limit set at: " << virtualMaxLimit << " pulses" << std::endl;


  // isso aqui é pra fazer ele voltar a posição inicial já prevendo a parede virtual, o prblema é q ele desliga o motor mas a inercia faz ele continuar andando e passa
  setBackward(CALIBRATION_SPEED);
  while (pulseCount.load() > virtualMinLimit) {
    updateEncoder();
    usleep(10000);
  }
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
  gpio.setPWMOutput(PWM_OUT, inputSpeed);
}

void MotorController::setBackward(int inputSpeed) {
  gpio.setDigitalOutput(DIR1, false);
  gpio.setDigitalOutput(DIR2, true);
  gpio.setPWMOutput(PWM_OUT, inputSpeed);
}

void MotorController::brake() {
  gpio.setDigitalOutput(DIR1, true);
  gpio.setDigitalOutput(DIR2, true);
  gpio.setPWMOutput(PWM_OUT, 0);
  speed = 0;
}

void MotorController::setSpeed(int inputSpeed) {
  gpio.setPWMOutput(PWM_OUT, inputSpeed);
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
