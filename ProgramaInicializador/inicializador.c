// Para shmget
#include <sys/ipc.h>
#include <sys/shm.h>
// Demas
#include <stdio.h>

int inicializador() {
    // En cada unidad de memoria compartida se guardara el siguiente string: ID<idproceso>
    // Debido a esto, cada unidad de memoria representara 4 bytes
    int sharedMemorySize;

    printf("Ingrese el tamanno de la memoria compartida: ");
    scanf("%d", &sharedMemorySize);
    printf("Memory Size %d \n", sharedMemorySize);

    int shmid = shmget(1234, sharedMemorySize*4, IPC_CREAT | 0666);
    
    return shmid;
}