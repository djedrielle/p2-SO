#include "ProgramaInicializador/inicializador.h"
#include <stdio.h>

int main() {

    int shmid = inicializador();

    printf("Shmid %d \n", shmid);

    return 0;
}