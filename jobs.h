#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include "pmon.h"

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
    int notificar;          // 1 si termino pero aun no se a avisado al usuario
} Job;

int agregar_job(pid_t pid, const char *comando); // agrega job a la tabla, devuelve su id o -1 si esta llena la tabla

void listar_jobs(void);

/*
busca entre los trabajos en segundo plano aquellos que tengan el mismo PID que termino, si lo encuentra y estaba activo, lo desactiva (activo=0)
a la vez activa una alerta (notificar=1) para poder avisarle al usuario mas adelante

y si el PID no se encuentra en la lista es porque era un proceso foreground, por lo tanto se ignora
*/
void marcar_terminado(pid_t pid);

/*
Imprime una notificacion "Done" para cada job recien terminado sin notificar, y lo marca como notificado
*/
void avisar_jobs_terminados(void);

int extraer_jobs_para_pmon(Proceso *lista_pmon);
#endif