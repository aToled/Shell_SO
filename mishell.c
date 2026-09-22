#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

           char *args[64];
           int i = 0;
           char *token = strtok(line, " \t\n");

           while (token != NULL && i < 63) {
               args[i] = token;
               i++;
               token = strtok(NULL, " \n");
           }
           args[i] = NULL;

        if (args[0] == NULL) {
            continue; 
        }
        
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork() error");
            exit(EXIT_FAILURE);

         } else if (pid == 0) {
             execvp(args[0], args);
            perror("execvp() error");
            exit(EXIT_FAILURE);
        
        } else {
             waitpid(pid, NULL, 0);
        }
    }
    
     free(line);
    return 0;

}