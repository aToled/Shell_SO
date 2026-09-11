#include <stdio.h>
#include <unistd.h> // Para getpid(), sleep() y sysconf()
#include "pmon.h"
#include <signal.h> // ¡Nueva librería para señales!


// 1. VARIABLES GLOBALES
// Las sacamos del main para que el manejador pueda acceder a ellas
Proceso proceso_real;
int mi_pid;
long hertz;

// 2. EL MANEJADOR DE LA ALARMA
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

    // ¡Importante! Volvemos a programar la alarma para el próximo segundo
    alarm(1);
}
// EL MANEJADOR DE SALIDA (Ctrl+C)
void manejador_salida(int sig) {
    (void)sig; // El mismo truco para que el compilador no se queje
    
    // Limpiamos la pantalla por última vez antes de irnos
    printf("\033[H\033[J");
    printf("Saliendo del monitor de procesos...\n");
    
    // Terminamos el programa de forma exitosa (0)
    _exit(0); 
}

// 3. EL MAIN (El Jefe)
int main() {
    mi_pid = getpid();
    hertz = sysconf(_SC_CLK_TCK);
    proceso_real.pid = mi_pid;

    // Llenamos los datos iniciales
    extraer_datos_stat(mi_pid, &proceso_real);
    proceso_real.memoria_rss = extraer_memoria_status(mi_pid);
    
    // Le avisamos al Sistema Operativo: 
    // "Cuando suene SIGALRM, ejecuta la función manejador_alarma"
    signal(SIGALRM, manejador_alarma);
    signal(SIGINT, manejador_salida); // <--- Atrapamos el Ctrl+C

    // Activamos la primera alarma para que suene en 1 segundo
    alarm(1);

    // En lugar de usar sleep() que congela todo, usamos un ciclo infinito vacío.
    // pause() simplemente manda a dormir al programa hasta que llegue CUALQUIER señal.
    // Esto gasta 0% de CPU mientras espera.
    while (1) {
        pause(); 
    }

    return 0;
}