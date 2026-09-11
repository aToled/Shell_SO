
#include "pmon.h"
#include <stdio.h>
#include <stdlib.h> //strtoul
#include <string.h> // strncmp
#include <unistd.h> // necesario para get pid() y sysconf

//Leer /proc/[pid]/stat (El estado y el tiempo)
void extraer_datos_stat(int pid, Proceso *p) {
    char ruta[256];
    
    // 1. Armamos la dirección de texto
    // Esto junta el número del PID dentro de la ruta para que quede, por ejemplo, "/proc/4821/stat"
    sprintf(ruta, "/proc/%d/stat", pid);

    // 2. Abrimos el archivo en modo lectura ("r")
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        printf("No se pudo abrir el proceso %d (quizás ya terminó)\n", pid);
        return;
    }

    char temporal[256];
    char estado;
    unsigned long utime = 0, stime = 0;

    // 3. El ciclo con contador
    //Todo el contenido de este archivo está escrito en una sola línea, con valores separados simplemente por espacios.
    // Sabemos que los datos llegan hasta la posición 15, así que el ciclo termina ahí.
    for (int i = 1; i <= 15; i++) {
        
        // fscanf lee una "palabra" (hasta el próximo espacio) y la guarda en 'temporal'
        if (fscanf(archivo, "%s", temporal) != 1) {
            break; // Si por alguna razón falla la lectura, salimos del ciclo
        }

        // Filtramos por la posición
        if (i == 3) {
            // Posición 3: Estado del proceso (R, S, Z, T)[cite: 1]
            estado = temporal[0]; // Solo tomamos la primera letra
        } 
        else if (i == 14) {
            // Posición 14: Tiempo de CPU en modo usuario (utime)[cite: 1]
            utime = strtoul(temporal, NULL, 10); // Convierte el texto a un número largo
        } 
        else if (i == 15) {
            // Posición 15: Tiempo de CPU en modo sistema (stime)[cite: 1]
            stime = strtoul(temporal, NULL, 10);
        }
    }

    // 4. Cierre y limpieza
    fclose(archivo);

   // Reemplazamos el printf final por las flechas (->)
    // Guardamos el estado y le ponemos el '\0' para que sea un texto válido en C
    p->estado[0] = estado;
    p->estado[1] = '\0'; 
    p->utime_anterior = utime;
    p->stime_anterior = stime;
}




// leer /proc/[pid]/status(La memoria)
unsigned long extraer_memoria_status(int pid) {
    char ruta[256];
    
    // 1. Armamos la dirección de texto tal como en el archivo anterior
    sprintf(ruta, "/proc/%d/status", pid);

    // 2. Abrimos el archivo en modo lectura
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        printf("No se pudo abrir status para el proceso %d\n", pid);
        return 0;
    }

    char linea[256];
    unsigned long memoria_rss = 0;

    // 3. El ciclo de búsqueda línea por línea
    // fgets lee una línea completa y la guarda en 'linea'. Se detiene si llega al final del archivo.
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        
        // strncmp compara los primeros 6 caracteres de la línea con "VmRSS:"
        // Si son exactamente iguales, retorna 0
        if (strncmp(linea, "VmRSS:", 6) == 0) {
            
            // 4. Extraemos el número
            // sscanf lee el texto que ya tenemos guardado en 'linea'
            // %*s significa "lee esta palabra pero ignórala" (saltamos "VmRSS:")
            // %lu guarda nuestro número
            // %*s vuelve a ignorar el final (saltamos "kB")
            sscanf(linea, "%*s %lu %*s", &memoria_rss);
            
            // Como ya encontramos lo que queríamos, rompemos el ciclo para no leer de más
            break; 
        }
    }

    // 5. Cierre y limpieza
    fclose(archivo);

    return memoria_rss;
}
/*
int main() {
    // Puedes probar con el PID 1, que es el sistema principal de Linux, siempre existe.
    extraer_datos_stat(1); 
    return 0;
}
*/



void mostrar_monitor(Proceso proceso_actual) {
    // Limpiamos la pantalla
    printf("\033[H\033[J");
    
    // Imprimimos cabecera
    printf("%-10s %-10s %-15s %-15s\n", "PID", "ESTADO", "%CPU", "MEMORIA(KB)");
    printf("----------------------------------------------------\n");
    
    
    
    // Imprimimos extrayendo los datos con el punto (.)
    printf("%-10d %-10s %-15.2f %-15lu\n", 
           proceso_actual.pid, 
           proceso_actual.estado, 
           proceso_actual.porcentaje_cpu, 
           proceso_actual.memoria_rss);
}