#include "uartController.h"
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <span>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace std;

class UARTExercicio1 {
private:
  enum class Command : uint8_t {
    INT_REQUEST = 0xA1,
    FLOAT_REQUEST = 0xA2,
    STRING_REQUEST = 0xA3,
    INT_SEND = 0xB1,
    FLOAT_SEND = 0xB2,
    STRING_SEND = 0xB3,
  };

  UARTController uart_;
  const vector<uint8_t> MATRICULA = {8, 1, 5, 0};

  void request(Command command) {
    uart_.ensureOpen();
    array<uint8_t, 5> msg = {static_cast<uint8_t>(command), MATRICULA[0],
                             MATRICULA[1], MATRICULA[2], MATRICULA[3]};
    uart_.send(msg);
    uart_.ensureClosed();
  }

  void send_data(Command command, const span<uint8_t> data) {
    uart_.ensureOpen();

    // NOTE: colocar buffer de mensagem fora do método pode evitar alocações
    vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(command));
    msg.insert(msg.end(), data.begin(), data.end());
    msg.insert(msg.end(), MATRICULA.begin(), MATRICULA.end());

    uart_.send(msg);
    uart_.ensureClosed();
  }

  void request_into(Command command, span<uint8_t> buffer) {
    uart_.ensureOpen();
    request(command);
    ssize_t bytesRead = 0;
    while (bytesRead < buffer.size()) {
      ssize_t r = uart_.read_into(buffer.subspan(bytesRead));
      if (r > 0)
        bytesRead += r;
    }
    uart_.ensureClosed();
  }

public:
  UARTExercicio1(const string &portName, const speed_t baudRate)
      : uart_(portName, baudRate) {}

  int requestInt() {
    int num = 0;
    // NOTE: endianess pode dar problema aqui
    request_into(Command::INT_REQUEST,
                 span<uint8_t>(reinterpret_cast<uint8_t *>(&num), sizeof(num)));
    return num;
  }

  float requestFloat() {
    float num = 0;
    // NOTE: em tese float não tem endianess
    request_into(Command::FLOAT_REQUEST,
                 span<uint8_t>(reinterpret_cast<uint8_t *>(&num), sizeof(num)));
    return num;
  }

  // NOTE: não sei se dá pra ficar mais genérico
  std::string requestString() {
    uart_.ensureOpen();
    request(Command::STRING_REQUEST);
    int lido = 0;
    uint8_t len;
    uart_.read_into(span<uint8_t>(&len, 1));
    string str(len, '\0');
    while (lido < len) {
      ssize_t r = uart_.read_into(
          span<uint8_t>(reinterpret_cast<uint8_t *>(&str[lido]), len - lido));
      if (r > 0)
        lido += r;
    }
    return str;
  }
  void send(int num) {
    send_data(Command::INT_SEND,
              span<uint8_t>(reinterpret_cast<uint8_t *>(&num), sizeof(num)));
  }

  void send(float num) {
    send_data(Command::FLOAT_SEND,
              span<uint8_t>(reinterpret_cast<uint8_t *>(&num), sizeof(num)));
  }

  void send(string &str) {
    send_data(
        Command::STRING_SEND,
        span<uint8_t>(reinterpret_cast<uint8_t *>(str.data()), str.size()));
  }
};

template <typename T> void printResult(const T &result) {
  cout << "Resultado: " << result << endl;
}

int main() {
  UARTExercicio1 uart("/dev/serial0", B9600);

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
      printResult(uart.requestInt());
      break;
    case 2:
      printResult(uart.requestFloat());
      break;
    case 3:
      printResult(uart.requestString());
      break;
    case 4: {
      int num;
      cout << "Digite um inteiro: ";
      cin >> num;
      break;
    }
    case 5: {
      float num;
      cout << "Digite um float: ";
      cin >> num;
      break;
    }
    case 6: {
      string str;
      cout << "Digite uma string: ";
      cin.ignore();
      getline(cin, str);
      uart.send(str);
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
