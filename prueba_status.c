#include "pmon.h"
#include <stdio.h>
#include <unistd.h> // get pid()

int main() {
    // getpid() obtiene el PID de este mismo programa en ejecución
    int mi_propio_pid = getpid();
    
    printf("Iniciando prueba de lectura de memoria...\n");
    printf("El PID de este programa de prueba es: %d\n", mi_propio_pid);
    
    // Llamamos a tu función pasándole el PID
    extraer_memoria_status(mi_propio_pid);
    
    return 0;
}