#include "comandos_internos.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int es_comando_interno(const char *nombre) {
    return strcmp(nombre, "cd") == 0 || strcmp(nombre, "exit") == 0 || strcmp(nombre, "jobs") == 0;
}

// hace el cambio de directorio al destino indicado o a $HOME
static void comando_interno_cd(char *args[], int argc) {
    const char *destino;

    if (argc < 2 || args[1] == NULL) {
        destino = getenv("HOME");
        if (destino == NULL) {
            fprintf(stderr, "cd: no se pudo determinar $HOME\n");
            return;
        }
    } else {
        destino = args[1];
    }

    if (chdir(destino) != 0) {
        perror("cd");
    }
}

// Cierra la shell
static void comando_interno_exit(char *args[], int argc) {
    int codigo = 0;
    if (argc >= 2) {
        codigo = atoi(args[1]);
    }
    exit(codigo); // termina la shell sin fork()
}

// deriva el comando detectado a su funcion interna
void ejecutar_comando_interno(char *args[], int argc) {
    if (strcmp(args[0], "cd") == 0) {
        comando_interno_cd(args, argc);
    } else if (strcmp(args[0], "exit") == 0) {
        comando_interno_exit(args, argc);
    } else if (strcmp(args[0], "jobs") == 0) {
        listar_jobs();
    }
}