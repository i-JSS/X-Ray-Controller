#include "gpio.h"
#include <array>
#include <iostream>
#include <unistd.h>

void quit() {
  // Stop all motors and quit
  exit(0);
}

struct botao {
  int pino;
  std::string nome;
  bool estado;
};

int main(void) {
  GPIOController &gpio = GPIOController::getInstance();

  std::array<botao, 4> botoes = {
      botao{BOTAO_CIMA, "Cima", false},
      botao{BOTAO_BAIXO, "Baixo", false},
      botao{BOTAO_ESQ, "Esquerda", false},
      botao{BOTAO_DIR, "Direita", false}};

  for (const auto &botao : botoes) {
    gpio.configureInputPin(botao.pino, [botao]() {
      std::cout << "Botão " << botao.nome << " pressionado!" << std::endl;
    });
  }

  gpio.configureInterrupt(BOTAO_EMERGENCIA, quit);

  while (true) {
    for (auto &botao : botoes) {
      bool estadoAtual = digitalRead(botao.pino);
      if (estadoAtual != botao.estado) {
        botao.estado = estadoAtual;
        if (estadoAtual) {
          std::cout << "Botão " << botao.nome << " pressionado!" << std::endl;
        } else {
          std::cout << "Botão " << botao.nome << " liberado!" << std::endl;
        }
      }
    }
    usleep(50000);
  }
  return 0;
}
