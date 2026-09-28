#include "jobs.h"
#include <stdio.h>
#include <string.h>
#include "pmon.h"

static Job jobs[max_jobs];
static int cantidad_jobs = 0;
static int siguiente_job_id = 1;

int agregar_job(pid_t pid, const char *comando){
    if(cantidad_jobs >= max_jobs){
        fprintf(stderr, "miShell: Numero maximo de jobs en background alcanzado\n");
        return -1;
    }

    /*
    se registra el nuevo proceso en la tabla de jobs, se asigna el id del job y se guarda el pid del sistema,
    finalmente copia el comando y marca el job como activo.
    */
    Job *j = &jobs[cantidad_jobs++];
    j-> job_id = siguiente_job_id++;
    j -> pid = pid;
    strncpy(j->comando, comando, sizeof(j->comando) - 1);
    j->comando[sizeof(j->comando) - 1] = '\0';
    j->activo = 1;
    j->notificar = 0;
    return j->job_id;
}

void listar_jobs(void) {
    for (int i = 0; i < cantidad_jobs; i++) {
        if (jobs[i].activo) {
            printf("[%d] Ejecutando %s\n", jobs[i].job_id, jobs[i].comando);
        }
    }
}

void marcar_terminado(pid_t pid){
    for (int i = 0; i < cantidad_jobs; i++) {
        if (jobs[i].pid == pid && jobs[i].activo) {
            jobs[i].activo = 0;
            jobs[i].notificar = 1;
            return;
        }
    }
}
 
void avisar_jobs_terminados(void) {
    for (int i = 0; i < cantidad_jobs; i++) {
        if (!jobs[i].activo && jobs[i].notificar) {
            printf("[%d]+ Done %s\n", jobs[i].job_id, jobs[i].comando);
            jobs[i].notificar = 0;
        }
    }
}

int extraer_jobs_para_pmon(Proceso *lista_pmon) {
    int cont = 0;
    for (int i = 0; i < cantidad_jobs; i++) {
        if (jobs[i].activo) {
            lista_pmon[cont].pid = jobs[i].pid;
            
            strncpy(lista_pmon[cont].comando, jobs[i].comando, 255);
            lista_pmon[cont].comando[255] = '\0';
            
            cont++;
        }
    }
    return cont; // Retorna exactamente cuántos procesos en background están corriendo
}