#ifndef PMON_H
#define PMON_H

/* 
 * pmon.h
 * Módulo de monitoreo de procesos para la shell.
 */

typedef struct {
    int pid;
    char estado[5];
    unsigned long utime_anterior;
    unsigned long stime_anterior;
    unsigned long memoria_rss;
    float porcentaje_cpu;
} Proceso;

void mostrar_monitor(Proceso proceso_actual);

// Extrae el estado y los tiempos de CPU (utime, stime) desde /proc/[pid]/stat
void extraer_datos_stat(int pid, Proceso *p);

// Extrae la memoria residente aproximada (VmRSS) desde /proc/[pid]/status
unsigned long extraer_memoria_status(int pid);

void iniciar_monitor(int segundos);

#endif 