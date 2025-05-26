#include "uartController.h"
#include <atomic>
#include <csignal>

std::atomic<bool> running{true};
void handler(int) { running.store(false); }

int main(void) {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);

  // Loop principal
  // UARTController uart;
  while (true) {

    if (!running.load())
      break;
  }

  // Destrutores cuidam de fechar e parar o sistema graciosamente
  return 0;
}
