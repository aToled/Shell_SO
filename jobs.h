#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

/*
La tabla de procesos background ejecutados por la shell (R5)
la shell solo interactua con la tabla de jobs a traves de estas funciones
*/
#define max_jobs 64
typedef struct {
    int job_id;             // el numero de job 
    pid_t pid;              
    char comando[256];      // texto original de la linea, para mostrar cuando el job termina      
    int activo;             // 1 o 0 dependiendo si esta corriendo o si termino
} Job;

int agregar_job(pid_t pid, const char *comando); // agrega job a la tabla, devuelve su id o -1 si esta llena la tabla

void listar_jobs(void);

#endif