#ifndef PMON_H
#define PMON_H

/* 
 * pmon.h
 * Módulo de monitoreo de procesos para la shell.
 */

typedef struct {
    int pid;
    char comando[256];
    char estado[3];
    unsigned long utime_anterior;
    unsigned long stime_anterior;
    unsigned long memoria_rss;
    float porcentaje_cpu;
    int activo;
} Proceso;

void mostrar_monitor(Proceso *lista_procesos, int total_procesos);

// Extrae el estado y los tiempos de CPU (utime, stime) desde /proc/[pid]/stat
int extraer_datos_stat(int pid, Proceso *p, unsigned long *utime_nuevo, unsigned long *stime_nuevo);

// Extrae la memoria residente aproximada (VmRSS) desde /proc/[pid]/status
unsigned long extraer_memoria_status(int pid);

void iniciar_monitor(int segundos, Proceso *lista_procesos, int total_procesos);

#endif 