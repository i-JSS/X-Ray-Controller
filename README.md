# Trabalho 2 - 2025-1

Integrantes:

| Nome          | GitHub    | Matrícula |
| ------------- | --------- | --------- |
| André Silva   | Hunter104 | 221007813 |
| João Carvalho | i-JSS     | MATRICULA |

## Módulos

Gerenciamento de erros é feito através de exceções, onde possível foi colocada funcionalidade de fechamento em funções destrutoras,
assegurando que recursos sejam liberados corretamente de forma automática ao módulo sair de escopo de alguma forma (RAII).

- i2cController: Classe controladora de I2C, métodos para leitura, escrita e gerenciamento de
  handle
  - bmp280Controller: Classe controladora/driver do sensor BMP280, métodos para inicialização e fechamento, leitura de temperatura e pressão
- gpioControllar: Classe controladora de GPIO, Singleton, métodos para configuração e controle de pinos, funcionalidade inutilizada de callbacks
  - motorController: Classe controladora de motor, métodos para controle de posição, modo de operação, calibração. Cada motor utiliza uma worker thread para polling de encoders sem bloqueio
- uartController: Classe controladora de UART, métodos para leitura e escrita de dados, com overloads para maior conveniência dos tipos de dados
  - modbusController: Classe controladora de Modbus, define métodos públicos para inicialização, leitura, escrita e verificação de mensagens.
    interface pública consiste em incialização, leitura de todos os registradores (informação armazenada em um struct para fácil manipulação), e escrita singular de registrador
    com sobrecarga de operadores para diferentes tipos de dados.
- pidController: Classe que encapsula estado e funcionalidade de um controle PID, permitindo instanciação de múltiplos controladores com diferentes parâmetros e contextos de execução.
<!-- Não merjado ainda -->
- - easyLoggingpp: Biblioteca externa para logging, utilizada para registrar eventos e estados do sistema com intuito de aumentar a rastreabilidade e facilitar o debug.

## Compilação

### Dependências

O projeto depende apenas de uma biblioteca externa, que deve ser instalada antes de compilar o projeto:

- wiringPi: Biblioteca para controle de GPIO, UART e I2C na Raspberry Pi.

Mas além disso é necessário ter uma API para threads, para que o CMAKE possa
encontrar a biblioteca de threads correta para o sistema operacional utilizado.

### Instruções

O projeto utiliza CMake para compilação, o que permite fácil portabilidade e configuração de ambiente.
Cada módulo é compilado separadamente como biblioteca estática a fim de evitar recompilações desnecessárias
em caso de múltiplos executáveis, algo que pode, e foi, utilizado para testes intermediários de cada módulo.

Para compilar o projeto, execute os seguintes comandos no terminal da raspberry pi:

```bash
mkdir build
cd build
cmake ..
make
./main
```

Pode se utilizar a flag `-DCMAKE_BUILD_TYPE=Release` ao compilar com cmake para compilar em modo release, o que irá otimizar o código e deverá remover logs de debug,
ou `-DCMAKE_BUILD_TYPE=Debug` para compilar em modo debug, o que irá incluir logs de debug e informações adicionais de rastreio.
