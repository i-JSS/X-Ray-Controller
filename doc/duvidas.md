# Aula de dúvidas

date: 03/06/2025

Obs: algumas placas estão sem sensor de temperatura
Primeira coisa para fazer é checar se o sensor está funcionando (i2cdetect)

VSCode abre servidor node para usar ssh e enxe a raspberry pi

Inicialização: vai ter um botão de reset da placa em todos os dashboards,
programa auxiliar pra escrever zero nas gpios antes de utilizar o botão.

O dashboard web só atualiza com frequência de 0.5 segundos

wiringPiSetupGpio -> Considera numeração da bcm
wiringPiSetup -> Considera numeração da wiringPi

botões -> Usuário pode apertar os botões ao mesmo tempo,
quando os dois estiverem apertados o programa não vai fazer nada,
mas alguns botões físicos podem ter gangorra (direcional do xbox 360)

Cinemática -> descrever todo o sistema de coordenadas em todos os eixos
para relacionar as equações de posição 3d em cada um dos eixos, mapeando
as equações (matrizes), cinemática inversa para movimentar um robô não
cartesiano para um sistema cartesiano, cada ponto calcular a posição dos
eixos para cada ponto na sequência
