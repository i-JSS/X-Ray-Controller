#include "gpioController.h"
#include "pidController.h"
#include <algorithm>
#include <atomic>
#include <thread>

void consume() {
  // This function is intentionally left empty to avoid unused function warnings.
}

class MotorController {
  PIDController pidController;
  GPIOController &gpio = GPIOController::getInstance();
  const int PWM_OUT;
  const int DIR1;
  const int DIR2;
  const int ENCODER_A;
  const int ENCODER_B;
  const int MIN_SENSOR;
  const int MAX_SENSOR;
  int speed = 0;
  std::atomic<long long int> pulseCount{0};
  std::thread encoderThread;
  long long int trackLengthInPulses = -1;
  int trackLengthInCM;
  std::atomic<bool> stopEncoder{false};

public:
  MotorController(int PWM, int DIR1, int DIR2, int ENCODER_A, int ENCODER_B, int MIN_SENSOR, int MAX_SENSOR, int trackLengthInCM);
  void init();
  void calibrate();
  void setFree();
  void setForward(int inputSpeed);
  void setBackward(int inputSpeed);
  void brake();
  void setSpeed(int inputSpeed);
  int getSpeed() const;
  long long int getEncoderCount() const;
  void resetEncoderCount();
  ~MotorController();
};
