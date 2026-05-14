CC = gcc
CFLAGS = -Wall -pthread

all: inicializador productor espia finalizador

inicializador: inicializador.c comun.h
	$(CC) $(CFLAGS) -o inicializador inicializador.c

productor: productor.c comun.h
	$(CC) $(CFLAGS) -o productor productor.c

espia: espia.c comun.h
	$(CC) $(CFLAGS) -o espia espia.c

finalizador: finalizador.c comun.h
	$(CC) $(CFLAGS) -o finalizador finalizador.c

clean:
	rm -f inicializador productor espia finalizador

.PHONY: all clean
