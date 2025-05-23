#pragma once
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <system_error>
#include <termios.h>
#include <unistd.h>


#define SERIAL_PORT "/dev/serial0"

enum Comandos {
    MOVER_EIXO_X_ESQ_DIR = 0x00,
    MOVER_EIXO_X_CIM_BAI = 0x01,
    POSICOES_1_4 = 0x02,
    PROG_POSICAO = 0x03,
    CALIBRAR = 0x04,
};

enum Informacao {
    REG_VEL_X = 0x05,
    REG_VEL_Y = 0x09,
    REG_POS_X = 0x0D,
    REG_POS_Y = 0x11,
    REG_TEMP = 0x15,
    REG_PRESSAO = 0x19,
    REG_ESTADO = 0x1D
};

class UARTController {
  int uart0_filestream = -1;

public:
  UARTController();

  void send(std::span<uint8_t> data);

  void read_into(std::span<uint8_t> buffer);

  ~UARTController();
};
