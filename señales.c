#include "señales.h"
#include "jobs.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

pid_t foreground_pid = 0;

static void manejador_sigchld(int sig) {
    (void) sig;
    int status;
    pid_t pid;

    /*
    se revisan todos los procesos hijos que terminaron al mismo tiempo y 
    se usa WNOHANG para limpiar la lista de trabajos para no quedar con trabajos esperando
    */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        marcar_terminado(pid);
    }
}

void instalar_manejador_sigchld(void) {
    struct sigaction sa;
    sa.sa_handler = manejador_sigchld;
    sigemptyset(&sa.sa_mask);             // limpia mascaras de señales
    sa.sa_flags = SA_RESTART;             // para evitar que getline() falle por interrupcion
    sigaction(SIGCHLD, &sa, NULL);
}

void ignorar_sigint_shell(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}
 
void restaurar_sigint_hijo(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}