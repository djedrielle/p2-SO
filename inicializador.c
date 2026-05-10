// Para shmget
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // En cada unidad de memoria compartida se guardara el siguiente string: ID<idthread>
    // Debido a esto, cada unidad de memoria representara 4 bytes
    int sharedMemorySize;
    int shmid;

    printf("Ingrese el tamanno de la memoria compartida: ");
    scanf("%d", &sharedMemorySize);
    printf("Memory Size %d \n", sharedMemorySize);

    // Crear la seccion de memoria compartida
    shmid = shmget(1234, sharedMemorySize*4, IPC_CREAT | 0666);
    
    // Adjuntar la memoria un momento para inicializarla (llenarla con ceros) usando memset
    char *memoria = (char *) shmat(shmid, NULL, 0);
    if (memoria != (char *)-1) {
        memset(memoria, 0, sharedMemorySize * 4);
        shmdt(memoria); // desconectamos de este proceso
    }
    
    // Guardar el shmid y el tamaño de la memoria en datos.txt
    FILE *archivo = fopen("datos.txt", "w");
    if (archivo != NULL) {
        fprintf(archivo, "Id Memoria Compartida: %d\n", shmid);
        fprintf(archivo, "Tamanno Memoria: %d\n", sharedMemorySize);
        fprintf(archivo, "Producir: true\n"); // Dejar la bandera encendida al arrancar
        fclose(archivo);
    } else {
        printf("Error: No se pudo abrir datos.txt para escribir\n");
    }
    
    return 0;
}