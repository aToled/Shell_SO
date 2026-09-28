#include "pmon.h"
#include <stdio.h>
#include <stdlib.h> //strtoul
#include <string.h> // strncmp
#include <unistd.h> // get pid() y sysconf
#include <signal.h> // sigaction y sig_atomic_t


//banderas atomicas
volatile sig_atomic_t actualizar_pantalla = 1; //1 para imprimir inmediatamente
volatile sig_atomic_t salir_monitor = 0;

//manejadores de señales
void manejador_alarma(int sig) {
    (void)sig;
    actualizar_pantalla = 1;
}

void manejador_salida(int sig) {
    (void)sig;
    salir_monitor = 1;
}
//Leer /proc/[pid]/stat (El estado y el tiempo)
int extraer_datos_stat(int pid, Proceso *p, unsigned long *utime_nuevo, unsigned long *stime_nuevo) {
    char ruta[256];

    //armamos la dirección de texto
    // Esto junta el número del PID dentro de la ruta para que quede, por ejemplo, "/proc/4821/stat"
    sprintf(ruta, "/proc/%d/stat", pid);

    //abrimos el archivo en modo lectura
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        p->activo = 0; //el proceso terminó, lo marcamos para no dibujarlo
        return 0;
    }

    char temporal[256];

    //ciclo con contador
    //Todo el contenido de este archivo está escrito en una sola línea, con valores separados simplemente por espacios.
    // Sabemos que los datos llegan hasta la posición 15, así que el ciclo termina ahí.
    for (int i = 1; i <= 15; i++) {
        
        // fscanf lee una "palabra" (hasta el próximo espacio) y la guarda en 'temporal'
        if (fscanf(archivo, "%s", temporal) != 1) {
            break;
        }

        //se filtra por la posición
        if (i == 3) {
            // Posición 3: Estado del proceso (R, S, Z, T)
            p->estado[0] = temporal[0];
            p->estado[1] = '\0';
        } 
        else if (i == 14) {
            // Posición 14: Tiempo de CPU en modo usuario (utime)
            *utime_nuevo = strtoul(temporal, NULL, 10); // Convierte el texto a un número largo
        } 
        else if (i == 15) {
            // Posición 15: Tiempo de CPU en modo sistema (stime)
            *stime_nuevo = strtoul(temporal, NULL, 10);
        }
    }
    fclose(archivo);
    return 1;
}

// leer /proc/[pid]/status(La memoria)
unsigned long extraer_memoria_status(int pid) {
    char ruta[256];
    
    //armamos la dirección de texto tal como en el archivo anterior
    sprintf(ruta, "/proc/%d/status", pid);

    // abrir el archivo en modo lectura
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        printf("No se pudo abrir status para el proceso %d\n", pid);
        return 0;
    }

    char linea[256];
    unsigned long memoria_rss = 0;

    //ciclo de búsqueda línea por línea
    // fgets lee una línea completa y la guarda en 'linea'. Se detiene si llega al final del archivo.
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        
        // strncmp compara los primeros 6 caracteres de la línea con "VmRSS:"
        // Si son exactamente iguales, retorna 0
        if (strncmp(linea, "VmRSS:", 6) == 0) {
            
            //se extrae el número
            // sscanf lee el texto que ya tenemos guardado en 'linea'
            // %*s significa "lee esta palabra pero ignórala" (saltamos "VmRSS:")
            // %lu guarda nuestro número
            // %*s vuelve a ignorar el final (saltamos "kB")
            sscanf(linea, "%*s %lu %*s", &memoria_rss);
            break; 
        }
    }
    fclose(archivo);

    return memoria_rss;
}
//funcion para ordenar procesos por %CPU de mayor a menor
int comparar_cpu(const void *a, const void *b) {
    Proceso *p1 = (Proceso *)a;
    Proceso *p2 = (Proceso *)b;
    
    // Validaciones para empujar procesos inactivos al fondo
    if (!p1->activo && p2->activo) return 1; 
    if (p1->activo && !p2->activo) return -1;

    if (p1->porcentaje_cpu < p2->porcentaje_cpu) return 1;
    if (p1->porcentaje_cpu > p2->porcentaje_cpu) return -1;
    return 0;
}

void mostrar_monitor(Proceso *lista_procesos, int total_procesos) {
    //ordenar el arreglo usando qsort y nuestra función comparadora
    qsort(lista_procesos, total_procesos, sizeof(Proceso), comparar_cpu);

    // Limpiamos la pantalla
    printf("\033[H\033[J");
    
    // CORRECCIÓN: Cabecera con 5 columnas (incluye COMANDO)
    printf("%-10s %-20s %-10s %-15s %-15s\n", "PID", "COMANDO", "ESTADO", "%CPU", "MEMORIA(KB)");
    printf("------------------------------------------------------------------------\n");
    
    for (int i = 0; i < total_procesos; i++) {
        // CORRECCIÓN: Si el proceso ya terminó, no se dibuja en la tabla
        if (!lista_procesos[i].activo) continue; 

        //resaltar el proceso con mayor uso
        if (i == 0 && total_procesos > 0) {
            printf("\033[1;36m"); 
        }
        
        // CORRECCIÓN: Imprimir los 5 datos, incluyendo lista_procesos[i].comando
        printf("%-10d %-20s %-10s %-15.2f %-15lu\n", 
               lista_procesos[i].pid, 
               lista_procesos[i].comando,
               lista_procesos[i].estado, 
               lista_procesos[i].porcentaje_cpu, 
               lista_procesos[i].memoria_rss);
               
        //apagar el color después de imprimir la primera fila
        if (i == 0 && total_procesos > 0) {
            printf("\033[0m");
        }
    }
}
void iniciar_monitor(int segundos,Proceso *lista_procesos, int total_procesos) {
    long hertz = sysconf(_SC_CLK_TCK);
    int intervalo = (segundos > 0) ? segundos : 2;

    struct sigaction sa_alarma, sa_salida, sa_ign;

    sa_alarma.sa_handler = manejador_alarma;
    sa_alarma.sa_flags = SA_RESTART;
    sigemptyset(&sa_alarma.sa_mask);
    sigaction(SIGALRM, &sa_alarma, NULL);

    sa_salida.sa_handler = manejador_salida;
    sa_salida.sa_flags = 0; 
    sigemptyset(&sa_salida.sa_mask);
    sigaction(SIGINT, &sa_salida, NULL);

    actualizar_pantalla = 1;
    salir_monitor = 0;

    // Inicializar lecturas base para todos los procesos recibidos
    for (int i = 0; i < total_procesos; i++) {
        lista_procesos[i].activo = 1;
        extraer_datos_stat(lista_procesos[i].pid, &lista_procesos[i], 
                           &lista_procesos[i].utime_anterior, 
                           &lista_procesos[i].stime_anterior);
    }

    while (!salir_monitor) {
        if (actualizar_pantalla) {
            actualizar_pantalla = 0; 

            for (int i = 0; i < total_procesos; i++) {
                if (!lista_procesos[i].activo) continue;

                unsigned long utime_nuevo = 0, stime_nuevo = 0;
                
                // Extrae datos. Si retorna 1, el proceso sigue vivo y calculamos el %CPU
                if (extraer_datos_stat(lista_procesos[i].pid, &lista_procesos[i], &utime_nuevo, &stime_nuevo)) {
                    lista_procesos[i].memoria_rss = extraer_memoria_status(lista_procesos[i].pid);
                    
                    unsigned long delta_utime = utime_nuevo - lista_procesos[i].utime_anterior;
                    unsigned long delta_stime = stime_nuevo - lista_procesos[i].stime_anterior;
                    
                    lista_procesos[i].porcentaje_cpu = (((delta_utime + delta_stime) / (float)hertz) / intervalo) * 100.0;
                    
                    lista_procesos[i].utime_anterior = utime_nuevo;
                    lista_procesos[i].stime_anterior = stime_nuevo;
                }
            }

            mostrar_monitor(lista_procesos, total_procesos);
            alarm(intervalo);
        }
        pause();
    }

    // Restaurar SIGINT a ser ignorado por la shell principal al salir (Solución Bug 6)
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = 0;
    sigaction(SIGINT, &sa_ign, NULL);

    
    //salida limpia sin matar la shell
    printf("\nSaliendo de pmon, devolviendo el control a miShell...\n");
}