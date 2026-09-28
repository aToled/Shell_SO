#ifndef SEÑALES_H
#define SEÑALES_H
 
/*
Manejo de SIGCHLD (R5) "recoleccion del estado de término de forma asíncrona
para recoger todos los hijos terminados sin bloquear la shell ni dejar procesos zombie"
 
Instala el manejador de SIGCHLD en un ciclo (manejador_sigchld), para recoger los hijos que ya terminaron sin bloquear la shell, ademas, 
por cada pid recogido intenta marcarlo como terminado en la tabla de jobs y si el pid no era un job en background se ignora. 
*/
void instalar_manejador_sigchld(void);

// se ejecuta cuando se inicializa la shell para que ignore a CTRL+C y CTRL+\.
void ignorar_sigint_shell(void);
 
/*
esto esta para ser llamado en el hijo tras el fork() pero antes del execvp solamente si el comando es foreground. Lo cual restaura
SIGINT/SIGQUIT a su comportamiento normal, si el comando es background no se llama la funcion para conservar el SIG_IGN
heredado de la shell y asi no ser afectado por CTRL+C
*/
void restaurar_sigint_hijo(void);
#endif