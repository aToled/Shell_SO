#include <stdio.h>
#include <unistd.h> // Para getpid(), sleep() y sysconf()
#include "pmon.h"
#include <signal.h>


// Las sacamos del main para que el manejador pueda acceder a ellas
Proceso proceso_real;
int mi_pid;
long hertz;

//el manejador de la alarma
// Esta función se ejecuta sola cada vez que pasa 1 segundo
void manejador_alarma(int sig) {
    (void)sig;
    // Tomamos la "nueva foto"
    extraer_datos_stat(mi_pid, &proceso_real);
    proceso_real.memoria_rss = extraer_memoria_status(mi_pid);

    // Calculamos el %CPU (asumiendo que ya teníamos un historial previo)
    unsigned long delta_utime = proceso_real.utime_anterior - proceso_real.utime_anterior; // Ojo aquí
    unsigned long delta_stime = proceso_real.stime_anterior - proceso_real.stime_anterior;

    proceso_real.porcentaje_cpu = ((delta_utime + delta_stime) / (float)hertz) * 100.0;

    // Imprimimos la tabla
    mostrar_monitor(proceso_real);

    //se vuelve a programar la alarma para el próximo segundo
    alarm(1);
}
//el manejador de salida (Ctrl+C)
void manejador_salida(int sig) {
    (void)sig; 
    printf("\033[H\033[J");
    printf("Saliendo del monitor de procesos...\n");
    _exit(0); 
}

int main() {
    mi_pid = getpid();
    hertz = sysconf(_SC_CLK_TCK);
    proceso_real.pid = mi_pid;

    // Llenamos los datos iniciales
    extraer_datos_stat(mi_pid, &proceso_real);
    proceso_real.memoria_rss = extraer_memoria_status(mi_pid);
    
    //cuando suene SIGALRM, ejecuta la funcion manejador_alarma
    signal(SIGALRM, manejador_alarma);
    signal(SIGINT, manejador_salida); //se atrapa el Ctrl+C

    //se activa la primera alarma 
    alarm(1);

    // pause() manda a dormir al programa hasta que llegue cualquier señal
    while (1) {
        pause(); 
    }

    return 0;
}