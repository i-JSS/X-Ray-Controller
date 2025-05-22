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
