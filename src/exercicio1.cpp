#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <utility>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace std;

class Uart {
public:

  array<uint8_t, 4> MATRICULA = {8, 1, 5, 0};
  enum class Command : uint8_t {
    INT_REQUEST = 0xA1,
    FLOAT_REQUEST = 0xA2,
    STRING_REQUEST = 0xA3,
    INT_SEND = 0xB1,
    FLOAT_SEND = 0xB2,
    STRING_SEND = 0xB3,
  };

  Uart(string portName, const speed_t baudRate)
      : portName(move(portName)), baudRate(baudRate) {}

  int request(Command comando) {
    const int fd = openSerial();
    const array<uint8_t, 5> msg = {static_cast<uint8_t>(comando), MATRICULA[0], MATRICULA[1], MATRICULA[2], MATRICULA[3]};
    if (write(fd, msg.data(), msg.size()) != 5) {
      throw std::system_error(errno, std::generic_category(), "Erro ao enviar via UART");
    }
    fsync(fd);
    usleep(100000);
    return fd;
  }

  void requestInt() {
    const int fd = request(Command::INT_REQUEST);
    int valor = 0, lido = 0;
    while (lido < 4) {
      int r = read(fd, reinterpret_cast<unsigned char *>(&valor) + lido, 4 - lido);
      if (r > 0) lido += r;
    }
    cout << "Valor recebido (int): " << valor << endl;
    close(fd);
  }

  void requestFloat() {
    const int fd = request(Command::FLOAT_REQUEST);
    int lido = 0;
    float valor = 0;
    while (lido < 4) {
      int r = read(fd, reinterpret_cast<unsigned char *>(&valor) + lido, 4 - lido);
      if (r > 0) lido += r;
    }
    cout << "Valor recebido (float): " << valor << endl;
    close(fd);
  }

  void requestString() {
    const int fd = request(Command::STRING_REQUEST);
    int lido = 0;
    uint8_t len;
    if (read(fd, &len, 1) != 1) throw runtime_error("Erro ao ler tamanho da string");
    string str(len, '\0');
    while (lido < len) {
      int r = read(fd, &str[lido], len - lido);
      if (r > 0) lido += r;
    }
    cout << "String recebida: " << str << endl;
    close(fd);
  }

  void sendInt(int num) {
    const int fd = openSerial();
    const array<uint8_t, 9> msg = {static_cast<uint8_t>(Command::INT_SEND),
      static_cast<uint8_t>((num >> 24)),
      static_cast<uint8_t>((num >> 16)),
      static_cast<uint8_t>((num >> 8)),
      static_cast<uint8_t>(num),
      MATRICULA[0], MATRICULA[1], MATRICULA[2], MATRICULA[3]};
    if (write(fd, msg.data(), msg.size()) != 9) {
      throw std::system_error(errno, std::generic_category(), "Erro ao enviar via UART");
    }
    close(fd);
  }

  void sendFloat(int num) {
    const int fd = openSerial();

    uint8_t* bytes = reinterpret_cast<uint8_t*>(&num);

    array<uint8_t, 9> msg = {
      static_cast<uint8_t>(Command::FLOAT_SEND),
      bytes[3],
      bytes[2],
      bytes[1],
      bytes[0],
      MATRICULA[0], MATRICULA[1], MATRICULA[2], MATRICULA[3]
    };
    if (write(fd, msg.data(), msg.size()) != 9) {
      throw std::system_error(errno, std::generic_category(), "Erro ao enviar via UART");
    }
    close(fd);
  }

  void sendString(const string& str) {
    const int fd = openSerial();
    vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(Command::STRING_SEND));
    msg.push_back(static_cast<uint8_t>(str.size()));
    msg.insert(msg.end(), str.begin(), str.end());
    msg.insert(msg.end(), MATRICULA.begin(), MATRICULA.end());

    if (write(fd, msg.data(), msg.size()) != static_cast<int>(msg.size())) {
      throw std::system_error(errno, std::generic_category(), "Erro ao enviar via UART");
    }
    close(fd);
  }

private:
  string portName;
  speed_t baudRate;

  int openSerial() {
    const int fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1) {
      perror("Erro ao abrir UART");
      return -1;
    }

    termios tty{};
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
};

int main() {
  Uart uart("/dev/serial0", B9600);

  while (true) {
    cout << "\n--- Menu UART ---\n";
    cout << "1. Requisitar inteiro\n";
    cout << "2. Requisitar float\n";
    cout << "3. Requisitar string\n";
    cout << "4. Enviar inteiro\n";
    cout << "5. Enviar float\n";
    cout << "6. Enviar string\n";
    cout << "0. Sair\n";
    cout << "Escolha uma opcao: ";

    int opcao;
    cin >> opcao;

    switch (opcao) {
      case 1:
        uart.requestInt();
        break;
      case 2:
        uart.requestFloat();
        break;
      case 3:
        uart.requestString();
        break;
      case 4: {
        int num;
        cout << "Digite um inteiro: ";
        cin >> num;
        uart.sendInt(num);
        break;
      }
      case 5: {
        float num;
        cout << "Digite um float: ";
        cin >> num;
        uart.sendFloat(num);
        break;
      }
      case 6: {
        string str;
        cout << "Digite uma string: ";
        cin.ignore();
        getline(cin, str);
        uart.sendString(str);
        break;
      }
      case 0:
        cout << "Encerrando...\n";
        return 0;
      default:
        cout << "Opcao invalida. Tente novamente.\n";
        break;
    }
  }
}
