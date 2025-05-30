#include "pidController.h"

double PIDController::getControlSignal(double measuredOutput) {

  double error = reference - measuredOutput;

  accumulatedError += error;

  if (accumulatedError >= MAX_CONTROL_SIGNAL) {
    accumulatedError = MAX_CONTROL_SIGNAL;
  } else if (accumulatedError <= MIN_CONTROL_SIGNAL) {
    accumulatedError = MIN_CONTROL_SIGNAL;
  }

  double deltaError = error - prevError;

  double controlSignal = Kp * error + (Ki * samplePeriod) * accumulatedError + (Kd / samplePeriod) * deltaError;

  if (controlSignal >= MAX_CONTROL_SIGNAL) {
    controlSignal = MAX_CONTROL_SIGNAL;
  } else if (controlSignal <= MIN_CONTROL_SIGNAL) {
    controlSignal = MIN_CONTROL_SIGNAL;
  }

  prevError = error;

  return controlSignal;
}

void PIDController::setReference(double newReference) {
  reference = newReference;
}
