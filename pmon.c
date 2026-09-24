
#include "pmon.h"
#include <stdio.h>
#include <stdlib.h> //strtoul
#include <string.h> // strncmp
#include <unistd.h> // get pid() y sysconf
#include <signal.h> // sigaction y sig_atomic_t


Proceso proceso_real;
int mi_pid;
long hertz;
int intervalo_segundos = 2;

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
void extraer_datos_stat(int pid, Proceso *p) {
    char ruta[256];
    
    //armamos la dirección de texto
    // Esto junta el número del PID dentro de la ruta para que quede, por ejemplo, "/proc/4821/stat"
    sprintf(ruta, "/proc/%d/stat", pid);

    //abrimos el archivo en modo lectura
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        printf("No se pudo abrir el proceso %d (quizás ya terminó)\n", pid);
        return;
    }

    char temporal[256];
    char estado;
    unsigned long utime = 0, stime = 0;

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
            estado = temporal[0]; // Solo tomamos la primera letra
        } 
        else if (i == 14) {
            // Posición 14: Tiempo de CPU en modo usuario (utime)
            utime = strtoul(temporal, NULL, 10); // Convierte el texto a un número largo
        } 
        else if (i == 15) {
            // Posición 15: Tiempo de CPU en modo sistema (stime)
            stime = strtoul(temporal, NULL, 10);
        }
    }
    fclose(archivo);

    // Guardamos el estado
    p->estado[0] = estado;
    p->estado[1] = '\0'; 
    p->utime_anterior = utime;
    p->stime_anterior = stime;
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
    
    if (p1->porcentaje_cpu < p2->porcentaje_cpu) return 1;
    if (p1->porcentaje_cpu > p2->porcentaje_cpu) return -1;
    return 0;
}

void mostrar_monitor(Proceso *lista_procesos, int total_procesos) {
    //ordenar el arreglo usando qsort y nuestra función comparadora
    qsort(lista_procesos, total_procesos, sizeof(Proceso), comparar_cpu);
    // Limpiamos la pantalla
    printf("\033[H\033[J");
    printf("%-10s %-10s %-15s %-15s\n", "PID", "ESTADO", "%CPU", "MEMORIA(KB)");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < total_procesos; i++) {
        //resaltar el proceso con mayor uso
        if (i == 0 && total_procesos > 0) {
            printf("\033[1;36m"); 
        }
        
        printf("%-10d %-10s %-15.2f %-15lu\n", 
               lista_procesos[i].pid, 
               lista_procesos[i].estado, 
               lista_procesos[i].porcentaje_cpu, 
               lista_procesos[i].memoria_rss);
               
        //apagar el color después de imprimir la primera fila
        if (i == 0 && total_procesos > 0) {
            printf("\033[0m");
        }
    }
}
void iniciar_monitor(int segundos) {
    mi_pid = getpid(); // Por ahora monitoreamos la shell misma como prueba
    hertz = sysconf(_SC_CLK_TCK);
    proceso_real.pid = mi_pid;
    
    // Si el usuario pasa un argumento válido, lo usamos. Si no, queda en 2.
    if (segundos > 0) {
        intervalo_segundos = segundos;
    }

    //Configurar sigaction
    struct sigaction sa_alarma;
    sa_alarma.sa_handler = manejador_alarma;
    sa_alarma.sa_flags = SA_RESTART;
    sigemptyset(&sa_alarma.sa_mask);
    sigaction(SIGALRM, &sa_alarma, NULL);

    struct sigaction sa_salida;
    sa_salida.sa_handler = manejador_salida;
    sa_salida.sa_flags = 0; 
    sigemptyset(&sa_salida.sa_mask);
    sigaction(SIGINT, &sa_salida, NULL);

    // Reiniciar banderas por si el usuario entra a pmon varias veces
    actualizar_pantalla = 1;
    salir_monitor = 0;

    // Extraer datos iniciales para tener un punto de comparación en el primer cálculo
    extraer_datos_stat(mi_pid, &proceso_real);

    //El ciclo principal exigido
    while (!salir_monitor) {
        
        if (actualizar_pantalla) {
            actualizar_pantalla = 0; 
            Proceso foto_nueva;
            extraer_datos_stat(mi_pid, &foto_nueva);
            proceso_real.memoria_rss = extraer_memoria_status(mi_pid);
            proceso_real.estado[0] = foto_nueva.estado[0];

            // Cálculo matemático real del %CPU
            unsigned long delta_utime = foto_nueva.utime_anterior - proceso_real.utime_anterior;
            unsigned long delta_stime = foto_nueva.stime_anterior - proceso_real.stime_anterior;
            
            // Tiempo gastado en CPU / tiempo real transcurrido * 100
            proceso_real.porcentaje_cpu = (((delta_utime + delta_stime) / (float)hertz) / intervalo_segundos) * 100.0;

            // Actualizamos el historial para el siguiente segundo
            proceso_real.utime_anterior = foto_nueva.utime_anterior;
            proceso_real.stime_anterior = foto_nueva.stime_anterior;

            mostrar_monitor(&proceso_real, 1);

            //se reprograma la alarma para el siguiente ciclo
            alarm(intervalo_segundos);
        }
        
        //en pausa la CPU hasta que llegue una señal
        pause();
    }

    //salida limpia sin matar la shell
    printf("\nSaliendo de pmon, devolviendo el control a miShell...\n");
}