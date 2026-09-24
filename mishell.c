#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(void){
    char *line = NULL;
    size_t len = 0;
    char cwd[1024];

    while(1){

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

        
        for (int i = 0; i < num_cmds; i++) {
            waitpid(pids[i], NULL, 0);
        }
        
    }
    free(line);
    return 0;
   
}