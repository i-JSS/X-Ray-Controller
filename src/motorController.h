#include "gpioController.h"
#include "pidController.h"
#include <atomic>
#include <thread>

struct motorData {
  float speed;
  float distance;
};

class MotorController {
public:
  MotorController(int PWM, int DIR1, int DIR2, int ENCODER_A, int ENCODER_B, int MIN_SENSOR, int MAX_SENSOR, int trackLengthInCM, int speed);
  void init();
  void calibrate();
  void setForward() const;
  void setBackward() const;
  void brake() const;
  void resetEncoderCount();
  motorData getMotorData() const;
  bool onForwardLimit() const;
  bool onBackwardLimit() const;
  ~MotorController();

private:
  long long int virtualMinLimit;
  long long int virtualMaxLimit;
  bool prevA = false;
  bool prevB = false;
  double cmPerPulse;
  PIDController pidController;
  GPIOController &gpio = GPIOController::getInstance();
  const int PWM_OUT;
  const int DIR1;
  const int DIR2;
  const int ENCODER_A;
  const int ENCODER_B;
  const int MIN_SENSOR;
  const int MAX_SENSOR;
  std::atomic<long long int> pulseCount{0};
  std::thread encoderThread;
  long long int trackLengthInPulses = -1;
  int trackLengthInCM;
  std::atomic<bool> stopEncoder{false};
  int speed;
};
