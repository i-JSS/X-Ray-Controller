#pragma once

class PIDController {
private:
  static constexpr double DEFAULT_KP = 0.5;
  static constexpr double DEFAULT_KI = 0.05;
  static constexpr double DEFAULT_KD = 40;
  static constexpr int MAX_CONTROL_SIGNAL = 100.0;
  static constexpr int MIN_CONTROL_SIGNAL = -100.0;

  const double Kp;
  const double Ki;
  const double Kd;

  double reference = 0.0;
  int samplePeriod = 1.0;
  double accumulatedError, prevError = 0.0;

public:
  PIDController(double Kp, double Ki, double Kd) : accumulatedError(0.0), Kp(Kp), Ki(Ki), Kd(Kd) {}

  PIDController() : PIDController(DEFAULT_KP, DEFAULT_KI, DEFAULT_KD) {}

  double getControlSignal(double measuredOutput);
  void setReference(double newReference);
};
