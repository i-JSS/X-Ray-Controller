#include "motorController.h"
#include "easylogging++.h"
#include "gpioController.h"
#include <thread>
#include <unistd.h>

MotorController::MotorController(int PWM, int DIR1, int DIR2, int ENCODER_A, int ENCODER_B, int MIN_SENSOR, int MAX_SENSOR, int trackLengthInCM, int speed)
    : PWM_OUT(PWM), DIR1(DIR1), DIR2(DIR2), ENCODER_A(ENCODER_A), ENCODER_B(ENCODER_B),
      MIN_SENSOR(MIN_SENSOR), MAX_SENSOR(MAX_SENSOR), trackLengthInCM(trackLengthInCM), speed(speed) {
  gpio.configurePWMPin(PWM_OUT);
  gpio.configureOutputPin(DIR1);
  gpio.configureOutputPin(DIR2);
  gpio.configureInputPin(ENCODER_A);
  gpio.configureInputPin(ENCODER_B);
  gpio.configureInputPin(MIN_SENSOR);
  gpio.configureInputPin(MAX_SENSOR);
  init();
}

void MotorController::init() {
  LOG(INFO) << "Initializing motor controller.";
  LOG(DEBUG) << "Motor pins: PWM: " << PWM_OUT
             << ", DIR1: " << DIR1 << ", DIR2: " << DIR2
             << ", ENCODER_A: " << ENCODER_A << ", ENCODER_B: " << ENCODER_B
             << ", MIN_SENSOR: " << MIN_SENSOR << ", MAX_SENSOR: " << MAX_SENSOR;
  bool prevA = gpio.getDigitalInput(ENCODER_A);
  bool prevB = gpio.getDigitalInput(ENCODER_B);

  encoderThread = std::thread([this, prevA, prevB]() mutable {
    while (!stopEncoder.load()) {
      bool a = gpio.getDigitalInput(ENCODER_A);
      bool b = gpio.getDigitalInput(ENCODER_B);
      if (a && !prevA) {
        if (b)
          pulseCount.fetch_sub(1);
        else
          pulseCount.fetch_add(1);
      }
      prevA = a;
      prevB = b;
      std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
  });
}

void MotorController::calibrate() {
  LOG(INFO) << "Starting motor calibration. (PWM " << PWM_OUT << ")";
  constexpr int DIST_FROM_LIMIT_CM = 3;

  setBackward(speed);
  while (!gpio.getDigitalInput(MIN_SENSOR)) {
    usleep(1000);
  }
  brake();
  resetEncoderCount();

  setForward(speed);
  while (!gpio.getDigitalInput(MAX_SENSOR)) {
    usleep(1000);
  }
  brake();
  trackLengthInPulses = pulseCount.load();

  if (trackLengthInPulses == 0 || trackLengthInCM == 0)
    throw std::runtime_error("Calibration error");

  cmPerPulse = static_cast<double>(trackLengthInCM) / trackLengthInPulses;
  const long long int marginPulses = static_cast<long long int>(DIST_FROM_LIMIT_CM / cmPerPulse);

  virtualMinLimit = marginPulses;
  virtualMaxLimit = trackLengthInPulses - marginPulses;

  LOG(DEBUG) << "Track length in pulses: " << trackLengthInPulses
             << ", CM per pulse: " << cmPerPulse
             << ", Virtual min limit: " << virtualMinLimit
             << ", Virtual max limit: " << virtualMaxLimit;

  while (!onBackwardLimit()) {
    setBackward(calculateLimitSpeed());
    usleep(1000);
  }
  brake();

  LOG(INFO) << "Calibration complete. Motor (PWM " << PWM_OUT << ") ready.";
}

void MotorController::setForward(float pwm) const {
  gpio.setDigitalOutput(DIR1, true);
  gpio.setDigitalOutput(DIR2, false);
  gpio.setPWMOutput(PWM_OUT, pwm);
}

void MotorController::setBackward(float pwm) const {
  gpio.setDigitalOutput(DIR1, false);
  gpio.setDigitalOutput(DIR2, true);
  gpio.setPWMOutput(PWM_OUT, pwm);
}

void MotorController::brake() const {
  gpio.setDigitalOutput(DIR1, true);
  gpio.setDigitalOutput(DIR2, true);
  gpio.setPWMOutput(PWM_OUT, 0);
}

void MotorController::free() const {
  gpio.setDigitalOutput(DIR1, false);
  gpio.setDigitalOutput(DIR2, false);
  gpio.setPWMOutput(PWM_OUT, 0);
}

void MotorController::resetEncoderCount() {
  pulseCount.store(0);
}

motorData MotorController::getMotorData() const {
  const long long lastPulse = pulseCount.load();
  const auto lastTime = std::chrono::steady_clock::now();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  const long long currentPulse = pulseCount.load();
  const auto currentTime = std::chrono::steady_clock::now();
  const long long deltaPulses = currentPulse - lastPulse;
  const double elapsedSeconds = std::chrono::duration<double>(currentTime - lastTime).count();
  const double deltaCM = deltaPulses * cmPerPulse;
  const double speed_mps = (deltaCM / 100.0) / elapsedSeconds;
  const double totalDistanceMeters = (currentPulse * cmPerPulse) / 100.0;

  return motorData{
      .speed = static_cast<float>(speed_mps),
      .distance = static_cast<float>(totalDistanceMeters)};
}

bool MotorController::onForwardLimit() const {
  return pulseCount.load() > virtualMaxLimit - virtualMinLimit;
}

bool MotorController::onBackwardLimit() const {
  return pulseCount.load() < virtualMinLimit * 2;
}

float MotorController::calculateLimitSpeed() {
  int pulse = pulseCount.load();
  if (pulse < virtualMinLimit * 6 || pulse < virtualMaxLimit - virtualMinLimit * 5)
    return speed / 2;
  return speed;
}


MotorController::~MotorController() {
  stopEncoder.store(true);
  if (encoderThread.joinable()) {
    encoderThread.join();
  }
  brake();
}
