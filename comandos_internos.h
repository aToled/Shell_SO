#ifndef COMANDOS_INTERNOS_H
#define COMANDOS_INTERNOS_H

/*
Comandos internos de la shell: cd, exit, jobs (R2)
*/

int es_comando_interno(const char *nombre);

void ejecutar_comando_interno(char *args[], int argc);

#endif