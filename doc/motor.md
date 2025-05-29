# Motor

data: 29/05/2025

Motor não tem zona morta, atrito estático, assim
pwm pode não ser o suficiente pra ultrapassar o atrito inicial (minimo pra iniciar).
Motor tem histerese/inércia

Movimentos:

- Botão apertado: aciona e acelera até bater a barreira virtual,
  Se apertar na direção oposta, acelera na direção oposta.
- Pré programação: se movimenta até posição marcada
  - Ou freiar o motor ao alcançar o alvo, mas ele não vai parar na posição (controle de malha aberta, sem informação em tempo real),
    - Regra de três? não funciona se a dinâmica da máquina mudar
  - Controle de malha fechada: realimento a saída na entrada, assim o sistema
    tem conhecimento do estado
    - Controle ON/OFF: utilizado em ar-condicionado, continuar enquanto condição existir
      parar caso contrário, para não ficar ligando e desligando utilizar limiar onde há mudanças
      (histerese, schmidtt trigger, geralmente de 1 grau), não há controle de potência. Alguns
      ACs tem inversores de potência que tem maior economia de energia. Não vai funcionar para o motor
    - Controle P.I.D: Proporcional, Integral e Derivativo, três estratégias de controle
      unidas. Controlador: referência - estado atual = erro. Existe tempo de amostragem para ajuste (discreto no tempo),
      meio segundo.

## PID

Constante deve ser experimental, a não
ser que o sistema possa ser modelado matematicamente.
Três dimensões independentes:

Proporcional: Constante \* erro.

Máquinas grandes e lentas não precisam de frequência de amostragem muito alta

tempo de amostragem deverá ser de 1Hz

Proporcional: Velocidade proporcional à distância, pode ficar lento. Mas
queremos chegar o mais rápido possíve.
Derivativo: Constante derivativa multiplicada pela derivada do erro (variação em relação ao tempo).
para fazer de maneira analítica é necessária a equação da curva, dá para fazer a variação com os pontos
pf - pi / dt, logo se estivermos chegando muito rápido, a derivada vai ser alta e o motor vai desacelerar,
pois a derivada é negativa (erro diminui).
Integral: pode acontecer um fenômeno de nunca chegar no alvo, pois o erro diminui muito.
Vai somando o erro ao longo do tempo, multiplicado pela constante integral a faz a máquina chegar ao ponto.

Funciona bem para sistemas dinâmcos pois o erro é sempre realimentado.

Não funciona bem para sistemas não lineares.
