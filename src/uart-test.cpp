#include "modbusController.h"

#include <csignal>
#include <cstdint>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

ModbusController modbus("/dev/serial0", B115200);

void closeProgram(int signal) {
  modbus.ensureClosed();
  // TODO: Fechar GPIO/I2C e desligar atuadores
  exit(0);
}

inline void printRequest(ModbusController::SubCode subcode, const std::string &prefix) {
  std::cout << prefix << std::endl;
  std::cout << "Valor inteiro: " << modbus.requestRead(subcode) << std::endl;
}

inline void printRequest(ModbusController::SubCode subcode, const std::string &prefix, float content) {
  auto data = reinterpret_cast<uint8_t *>(&content);
  std::cout << prefix << std::endl;
  std::cout << "Valor inteiro: " << modbus.requestWrite(subcode, std::span<uint8_t>(data, sizeof(float)))
            << std::endl;
}

inline void printRequest(ModbusController::SubCode subcode, const std::string &prefix, uint8_t content) {
  std::cout << prefix << "Valor inteiro: " << std::endl;
  std::cout << modbus.requestWrite(subcode, std::span<uint8_t>(&content, 1))
            << std::endl;
}

int main() {
  std::signal(SIGINT, closeProgram);
  std::cout << "Programa rodando... (Ctrl+C para sair)\n\n";

  using SubCode = ModbusController::SubCode;

  printRequest(SubCode::MOVE_X_LEFT_RIGHT, "Mover Eixo-X (Esquerda / Direita): ");
  printRequest(SubCode::MOVE_Y_UP_DOWN, "Mover Eixo-X (Para Cima / Para Baixo): ");
  printRequest(SubCode::PRESET_POSITIONS, "Posições Pré-Definidas (1 a 4): ");
  printRequest(SubCode::SET_PRESET_POSITION, "Botão para Programar Posição Pré-definida: ");
  printRequest(SubCode::CALIBRATE, "Calibrar: ");

  std::cout << std::endl;

  struct {
    SubCode code;
    const char *label;
    float value;
  } floatOps[] = {
      {SubCode::REG_SPEED_X, "Velocidade Instantânea Eixo-X: ", 0.1f},
      {SubCode::REG_SPEED_Y, "Velocidade Instantânea Eixo-Y: ", 0.2f},
      {SubCode::REG_POSITION_X, "Posição Eixo-X: ", 0.3f},
      {SubCode::REG_POSITION_Y, "Posição Eixo-Y: ", 0.3f},
      {SubCode::REG_TEMPERATURE, "Temperatura do ambiente: ", 0.5f},
      {SubCode::REG_PRESSURE, "Pressão Atmosférica do ambiente: ", 0.6f}};

  for (const auto &op : floatOps)
    printRequest(op.code, op.label, op.value);

  std::cout << std::endl;

  printRequest(SubCode::REG_MACHINE_STATE, "Estado de Operação da Máquina: ", uint8_t{0x01});

  return 0;
}
