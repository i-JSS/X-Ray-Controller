#pragma once
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <wiringPi.h>

constexpr int MOTOR_X_PWM = 17;
constexpr int MOTOR_X_DIR1 = 27;
constexpr int MOTOR_X_DIR2 = 22;
constexpr int MOTOR_Y_PWM = 23;
constexpr int MOTOR_Y_DIR1 = 24;
constexpr int MOTOR_Y_DIR2 = 25;

constexpr int ENCODER_X_A = 5;
constexpr int ENCODER_X_B = 6;
constexpr int ENCODER_Y_A = 12;
constexpr int ENCODER_Y_B = 13;

constexpr int SENSOR_X_MIN = 26;
constexpr int SENSOR_X_MAX = 19;
constexpr int SENSOR_Y_MIN = 20;
constexpr int SENSOR_Y_MAX = 21;

constexpr int CAPTURA = 18;

constexpr int BOTAO_CIMA = 16;
constexpr int BOTAO_BAIXO = 1;
constexpr int BOTAO_ESQ = 7;
constexpr int BOTAO_DIR = 8;

constexpr int BOTAO_EMERGENCIA = 11;

typedef std::function<void(void)> callback_t;
class GPIOController {
private:
  // NOTE: criei meu própio enum pois
  // não tenho o wiringPi na minha máquina,
  // quero certificar que tudo funciona
  enum class Mode { IN,
                    OUT,
                    PWM_OUT,
                    INTERRUPT };
  struct Pin {
    int id;
    Mode mode;
  };

  struct InputPin : Pin {
    std::optional<bool> lastState;
    std::optional<callback_t> handle;
  };

  std::vector<Pin> configuredPins;

  GPIOController() {
    wiringPiSetupGpio();
  }

public:
  GPIOController(const GPIOController &) = delete;
  void operator=(const GPIOController &) = delete;
  static GPIOController &getInstance() {
    static GPIOController instance;
    return instance;
  }

  /* PERF: Espero encarecidamente que essas funções sejam inlined
   * pelo compilador
   */
  const Pin *getExistingPin(int pin);
  void configureInputPin(int pin, callback_t handle = nullptr);
  void configureOutputPin(int pin);
  void configurePWMPin(int pin);
  void configureInterrupt(int pin, void (*handle)(void));

  void setPWMOutput(int pin, int value);
  void setDigitalOutput(int pin, bool value);
  bool getDigitalInput(int pin) const;
};
