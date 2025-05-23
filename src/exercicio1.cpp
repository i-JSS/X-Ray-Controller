#include  <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <utility>

using namespace std;

enum command {
    SOLICITA_INT = 0xA1,
    SOLICITA_FLOAT = 0xA2,
    SOLICITA_STRING = 0xA3,
};

class Uart {
public:
    Uart(string portName, speed_t baudRate)
        : portName(std::move(portName)), baudRate(baudRate) {}

    int open_serial() {
        int fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (fd == -1) {
            perror("Erro ao abrir UART");
            return -1;
        }

        struct termios tty{};
        if (tcgetattr(fd, &tty) != 0) {
            perror("Erro ao obter atributos da UART");
            close(fd);
            return -1;
        }

        cfsetospeed(&tty, baudRate);
        cfsetispeed(&tty, baudRate);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~IGNBRK;
        tty.c_lflag = 0;
        tty.c_oflag = 0;

        tty.c_cc[VMIN]  = 1;
        tty.c_cc[VTIME] = 1;

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            perror("Erro ao aplicar configurações na UART");
            close(fd);
            return -1;
        }

        return fd;
    }

    void close_serial(int fd) {
        close(fd);
    }

    void request(unsigned char comando) {
        int fd = open_serial();
        if (fd < 0) return;

        unsigned char msg[5] = {comando, 8, 1, 5, 0};
        if (write(fd, msg, 5) != 5) {
            perror("Erro ao escrever na UART");
            close_serial(fd);
            return;
        }

        fsync(fd);
        usleep(100000);

        if (comando == SOLICITA_INT) {
            int valor = 0;
            int lido = 0;
            while (lido < 4) {
                int r = read(fd, ((unsigned char*)&valor) + lido, 4 - lido);
                if (r > 0) lido += r;
            }
            printf("Valor recebido (int): %d\n", valor);
        } else if (comando == SOLICITA_FLOAT) {
            float valor = 0;
            int lido = 0;
            while (lido < 4) {
                int r = read(fd, ((unsigned char*)&valor) + lido, 4 - lido);
                if (r > 0) lido += r;
            }
            printf("Valor recebido (float): %f\n", valor);
        } else if (comando == SOLICITA_STRING) {
            unsigned char len = 0;
            read(fd, &len, 1);
            char str[len + 1];
            int lido = 0;
            while (lido < len) {
                int r = read(fd, str + lido, len - lido);
                if (r > 0) lido += r;
            }
            str[len] = '\0';
            printf("String recebida: %s\n", str);
        }

        close_serial(fd);
    }

private:
    string portName;
    speed_t baudRate;
};

int main() {
    Uart uart("/dev/serial0", B9600);
    uart.request(SOLICITA_INT);
    uart.request(SOLICITA_FLOAT);
    uart.request(SOLICITA_STRING);

    return 0;
}
