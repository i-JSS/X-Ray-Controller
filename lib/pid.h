#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void pid_configura_constantes(double Kp_, double Ki_, double Kd_);
void pid_atualiza_referencia(float referencia_);
double pid_controle(double saida_medida);

#ifdef __cplusplus
}
#endif

