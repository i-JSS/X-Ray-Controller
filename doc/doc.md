# Avisos

software tem que ser organizado em camadas de abstração

- Driver de GPIO
- Driver UART -> Driver Modbus -> Driver mensagens
- Drivar de I2C

Padrão de projeto em sistemas embarcados:
Cada módulo geralmente consiste de três funções:

- Inicialização -> chamado no início da função main
- Loop (se houver máquina de estados)
- vinit/close

Funções de envio da UART:

- write (pode dar erro, erros são diferente de erros ModBUs)
- read (receber)

Modbus: montar pacote e dar significado aos bytes

Esse problema tem que ter um loop contínuo de polling de usuário,
Loop macro (modbus botão apertado e etc), mas o uart é muito mais rápido
então se der erro você pode tentar denovo algumas vezes,

Se der muito erro: pode tentar zerar o buffer de entrada (incomum)

últimos 4 bytes tem que ser os útimos 4 dígitos da matrícúla, ex:
0x07 0x08 0x01 0x03

Vamos conversar com o sensor bmp via I2C

## I2C

Protocolo síncrono, half duplex

UART -> barramento ponto a ponto
I2C -> SCK/SCL (clock), SDA (Dados).

Podem haver múltiplos dispositivos no mesmo barramento I2C,
geralmente utilize mestre escravo, mas suporta múltiplos mestres (não há curto
circuito devido à transistores que interrompem a comunicação).

Para evitar erros, a próxima escrita em caso de erro tem offset aleatório (
TCP/IP funciona assim)

Para identificar só é necessário indicar o endereço no começo da mensagem,
toda mensagem tem uma resposta (todo mundo espera a comunicação terminar)
protocolo tem um tempo (?) para evitar monopólio.

No projeto vamos utilizar um sensor BMP280, que é um sensor de pressão e
temperatura. Usado para navegação e tempo.
Precisão de +- 0.12 hPa (permite estimar altitude com precisão de 1m).

É possível fazer medidas sofisticadas como lançes de escada (com estimativas
de altura de lançes de escada) junto com altitude e número de passos.

O sensor é um sistema embarcado rodando um firmware, com um frontend (fronteira do circuito).

O sensor também tem um Driver

ler o datasheet

i2cdetect -> detecta todos os dispositivos no barramento i2c (-y 1),
fazer perguntas utilizando o driver, não vai dar tempo de implementar driver i2c
