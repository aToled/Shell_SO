#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "comandos_internos.h"
#include "jobs.h"
#include "pmon.h"
#include "señales.h"

int main(void){
    char *line = NULL;
    size_t len = 0;
    char cwd[1024];

    
    instalar_manejador_sigchld();
    ignorar_sigint_shell();
    while(1){
        avisar_jobs_terminados();

       // mostrar direccion
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("miShell:%s$ ", cwd);
            fflush(stdout);
        } else {
            perror("getcwd() error");
            exit(EXIT_FAILURE);
        }

        if (getline(&line, &len, stdin) == -1) {
            printf("\n");
            break;
        }

        size_t line_len = strlen(line); // quitar el \n que deja getline()
        while (line_len > 0 && (line[line_len-1]=='\n' || line[line_len-1]=='\r')) {
            line[--line_len] = '\0';
        }

        int es_background = 0;      // si detecta el '&' al final de la linea, entonces (R5)
        while (line_len > 0 && isspace((unsigned char)line[line_len-1])) {
            line[--line_len] = '\0';
        }
        if (line_len > 0 && line[line_len-1] == '&') {
            es_background = 1;
            line[--line_len] = '\0';
            while (line_len > 0 && isspace((unsigned char)line[line_len-1])) {
                line[--line_len] = '\0';
            }
        }

        // copia el comando original para usarlo como texto descriptivo del job, se toma antes de que el strtok destruya 'line' al separar las pipes
        char comando_original[256];
        strncpy(comando_original, line, sizeof(comando_original) - 1);
        comando_original[sizeof(comando_original) - 1] = '\0';

        if (line_len == 0) {
            continue;
        }

        char *cmds[64];
        int num_cmds = 0;
        char *cmd_token = strtok(line, "|");

        while (cmd_token != NULL && num_cmds < 64) {
            cmds[num_cmds++] = cmd_token;
            cmd_token = strtok(NULL, "|");
        }

        if ( num_cmds== 0) {
            continue;
        }
        char cmd_copia[256];
        strncpy(cmd_copia, cmds[0], sizeof(cmd_copia) - 1);
        cmd_copia[255] = '\0';
        
        /* Tokenizamos completo para poder revisar los comandos internos con sus argumentos (cd [dir], exit [n]) 
        ademas del primer token, esto solo aplica cuando la linea no es una pipe (num_cmds == 1). */
        char *args_comandos_internos[64];
        int arg_comando_interno = 0;

        {
            char *tok = strtok(cmd_copia, " \t\n");
            while (tok != NULL && arg_comando_interno < 63) {
                args_comandos_internos[arg_comando_interno++] = tok;
                tok = strtok(NULL, " \t\n");
            }
            args_comandos_internos[arg_comando_interno] = NULL;
        }

        if (arg_comando_interno == 0) {
            continue;
        }

        if (num_cmds == 1 && strcmp(args_comandos_internos[0], "pmon") == 0) {
            int segundos = 2; // Valor por defecto

            if (arg_comando_interno >= 2) {
                segundos = atoi(args_comandos_internos[1]);
            }

            iniciar_monitor(segundos);
            continue;
        }

        if (num_cmds == 1 && es_comando_interno(args_comandos_internos[0])) {
            ejecutar_comando_interno(args_comandos_internos, arg_comando_interno);     //"exit" termina aqui y no retorna
            continue;
        }

        int num_pipes = num_cmds - 1;
        int pipefds[2 * num_pipes];

        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefds + i * 2) < 0) {
                perror("pipe() error");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pids[64];

        for (int i = 0; i < num_cmds; i++) {
            pids[i] = fork();

            if (pids[i] < 0) {
                perror("fork() error");
                exit(EXIT_FAILURE);
            } else if (pids[i] == 0) {

                if(!es_background){ //aqui es donde el hijo hereda el comportamiento de SIGINT/SIGQUIT para que los procesos background queden 'protegidos' del CTRL+C
                    restaurar_sigint_hijo();
                }

                if (i > 0) {
                    dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
                }
                if (i < num_cmds - 1) {
                    dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
                }

                for (int j = 0; j < 2 * num_pipes; j++) {
                    close(pipefds[j]);
                }

                char *args[64];
                int arg_count = 0;
                char *infile = NULL;
                char *outfile = NULL;
                int append = 0;

                char *token = strtok(cmds[i], " \t\n");
                while (token != NULL && arg_count < 63) {
                    if (strcmp(token, "<") == 0) {
                        infile = strtok(NULL, " \t\n");
                    } else if (strcmp(token, ">") == 0) {
                        outfile = strtok(NULL, " \t\n");
                        append = 0; // Modo truncar
                    } else if (strcmp(token, ">>") == 0) {
                        outfile = strtok(NULL, " \t\n");
                        append = 1; // Modo append
                    } else {
                        args[arg_count++] = token; 
                    }
                    token = strtok(NULL, " \t\n");
                }

                args[arg_count] = NULL;

                if (args[0] == NULL) {
                    exit(EXIT_SUCCESS); 
                }

                if (infile != NULL) {
                    int fd_in = open(infile, O_RDONLY);
                    if (fd_in < 0) {
                        perror("open() error input");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                if (outfile != NULL) {
                    int flags = O_WRONLY | O_CREAT;
                    if (append) flags |= O_APPEND;
                    else flags |= O_TRUNC;
                    
                    int fd_out = open(outfile, flags, 0644);
                    if (fd_out < 0) {
                        perror("open() error output");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                execvp(args[0], args);
                perror("execvp() error");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < 2 * num_pipes; i++) {
            close(pipefds[i]);
        }

        if (es_background) {
            
            // se registra el job y el manejador de de SIGCHLD se encargara de recogerlo cuando eventualmente termine
            int job_id = agregar_job(pids[num_cmds-1], comando_original);
            if (job_id > 0){
                printf("[%d] %d\n", job_id, pids[num_cmds-1]);
            }
        } else {
            /*
            se espera hasta que terminen todos los procesos foreground (si el manejador fue mas rapido por temas de ejecuccion y este ya limpio alguno de
            los trabajos, entonces el waitpid() va a fallar, pero no importa ya que el proceso ya fue liberado y como no se hace nada con el codigo de salida
            la consola no se va a bloquear) 
            */
            for (int i = 0; i < num_cmds; i++) {
                waitpid(pids[i], NULL, 0);
            }
        }
    }
    free(line);
    return 0;
}