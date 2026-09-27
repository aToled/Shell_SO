CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11
 
OBJS = mishell.o jobs.o comandos_internos.o pmon.o
 
mishell: $(OBJS)
	$(CC) $(CFLAGS) -o mishell $(OBJS)
 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
 
clean:
	rm -f *.o mishell

.PHONY: clean