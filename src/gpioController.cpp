#include "gpioController.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <wiringPi.h>

// NOTE: se falta de referência a Pin for problema de performance
// pode trocar por iterador
const GPIOController::Pin *GPIOController::getExistingPin(int pin) {
  // PERF: busca linear + número constante de pinos = O(1)
  const auto &it = std::find_if(configuredPins.begin(), configuredPins.end(),
                                [pin](const Pin &p) { return p.id == pin; });

  return (it != configuredPins.end()) ? &(*it) : nullptr;
}

void GPIOController::configureInputPin(int pin, callback_t handle) {
  if (getExistingPin(pin))
    throw std::runtime_error("Pin" + std::to_string(pin) + " is already configured.");

  pinMode(pin, INPUT);

  std::optional<callback_t> optHandle = handle ? std::make_optional(handle) : std::nullopt;
  InputPin inputPin = {pin, Mode::IN, optHandle, handle};
}

void GPIOController::configureOutputPin(int pin) {
  if (getExistingPin(pin))
    throw std::runtime_error("Pin" + std::to_string(pin) + " is already configured.");

  pinMode(pin, OUTPUT);

  Pin outputPin = {pin, Mode::OUT};
  configuredPins.push_back(outputPin);
}

void GPIOController::configurePWMPin(int pin) {
  if (getExistingPin(pin))
    throw std::runtime_error("Pin" + std::to_string(pin) + " is already configured.");

  pinMode(pin, PWM_OUTPUT);

  Pin pwmPin = {pin, Mode::PWM_OUT};
  configuredPins.push_back(pwmPin);
}

void GPIOController::configureInterrupt(int pin, void (*handle)(void)) {
  if (getExistingPin(pin))
    throw std::runtime_error("Pin" + std::to_string(pin) + " is already configured.");

  wiringPiISR(pin, INT_EDGE_BOTH, handle);

  Pin interruptPin = {pin, Mode::INTERRUPT};
  configuredPins.push_back(interruptPin);
}

void GPIOController::setPWMOutput(int pin, int value) {
  pwmWrite(pin, value);
}

void GPIOController::setDigitalOutput(int pin, bool value) {
  digitalWrite(pin, value ? HIGH : LOW);
}

// TODO: adicionar ativar função de callback automaticamente
bool GPIOController::getDigitalInput(int pin) const {
  return digitalRead(pin) == HIGH;
}
