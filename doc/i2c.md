# I2C

data: 2025-05-27

É possível modificar o endereço de um dispositivo I2C,
para flexibilidade no uso com mais de um dispositivo.

BMP tem i2c e SPI

Modo de mudar endereço pode ser mais ou menos drástico:
Por exemplo: cortar a placa com estilete e soldar os fios.
Ou
Pinos: que mudam o endereço automaticamente

Endereços padrão: displays 02, sensores 07, etc. Não é
possível ter endereço fora do padrão. Então não dá pra
ter mais de um dispositivo do mesmo tipo. ->
Por isso a rasp tem mais de um canal I2C

Precisa ter calibração do sensor para ter acurácia

Sensores tem faixa de linearidade, onde se mantém o
comportamento linear, e portanto é possível medir a temperature
Se não, vai ter que criar uma tabela de conversão e utilizando
interpolação de dados

Na vida real não se pode usar bebezisse do python

Linux já vem com drivers para vários sensores: no bootloader
/boot/config.txt -> (sudo rasp config) -> driver=i2c-sensor.bmp280

/sys/bus/i2c/devices/i2c-1/1-8676/iio-devices -> fds com driver do kernel
/sys/bus/i2c/devices/i2c-1/1-8676/iio-devices/bmp/in_temp -> precisa fazer uma conversão
só fazer um read

## Dashboards

Botões físicos: led indica estado da GPIO

Painel touch -> Reg 0 -> 1

Motores -> leds indicam os limites
barra laranja mostra posição

Controle de velocidade do motor via pwm

Calcular posição e velocidade via pulsos do encoder e calibração

Motor tem inércia

Cada placa tem uma dimensão um pouco diferente, que deverá ser calculada via pulsos
(tabela com pulsos no gitlab)

Encoder, interrupção ou thread com polling

Equipamento deve ser responsivo e informações do dashboard devem ser atualizadas

## Modbus

Mapa de memória: endereços e o que eles representam no equipamento

## Motor

ON/OFF primitivo, vamos usar algoritmo de controle PID, já tem algoritmo no
repositório. Começar a controlar o equipamento

Por enquanto motores só ON/OFF nas raspberries

Fazer com wiringPi pra usar interrupção, bcm precisa de sudo.

## Sugestão de camadas

É preciso modularizar o código, pois facilita a implementação,
dá pra garantir que a lógica de baixo nível funciona, podendo subir o nível

Módulos:
GPIO -> camada que configura e monitora entradas e saídas
-> Botões: deve manter o estado
-> Encoder:  
-> Motor (PWM)
UART -> camada que configura e monitora comunicação serial
Modbus -> é o que monta os pacotes, pode separar em outro módulo com as mensagens
I2C -> camada que configura e monitora comunicação I2C
Sensores -> temperatura e pressão
Camada de controle -> Ordena tudo num loop e pode fazer coisa em paralelo, ou pode ser serial (simples)
Pode ter submódulos: ações -> callbacks por exemplo

## Medicina

empresa médica da unb quer colora bootloader em máquinas,
com segurança e criptografia para poder atualizar via ethernet
e atualizar o firmware e diminuir custo de deploy.
