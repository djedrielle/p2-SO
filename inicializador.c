// Para shmget
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <stdlib.h>

int main() {
    // En cada unidad de memoria compartida se guardara el siguiente string: ID<idproceso>
    // Debido a esto, cada unidad de memoria representara 4 bytes
    int sharedMemorySize;
    int shmid;

    printf("Ingrese el tamanno de la memoria compartida: ");
    scanf("%d", &sharedMemorySize);
    printf("Memory Size %d \n", sharedMemorySize);

    // Crear la seccion de memoria compartida y guardar su id 
    // en el archivo datos.txt
    shmid = shmget(1234, sharedMemorySize*4, IPC_CREAT | 0666);
    
    FILE *archivo = fopen("datos.txt", "w");
    if (archivo != NULL) {
        fprintf(archivo, "Id Memoria Compartida: %d", shmid);
        fclose(archivo);
    } else {
        printf("Error: No se pudo abrir datos.txt para escribir\n");
    }
    
    return 0;
}