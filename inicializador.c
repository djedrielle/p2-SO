// Programa Inicializador
// Crea el ambiente: memoria compartida (RAM + tabla de estado), semáforos y bitácora.
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include "comun.h"

int main() {
    int sharedMemorySize;
    int shmid_mem, shmid_estado;

    printf("Ingrese el tamanno de la memoria compartida: ");
    scanf("%d", &sharedMemorySize);
    printf("Memory Size %d \n", sharedMemorySize);

    // Crear segmento de memoria compartida para la RAM simulada
    shmid_mem = shmget(KEY_MEMORIA, sharedMemorySize * CELL_SIZE, IPC_CREAT | 0666);
    if (shmid_mem == -1) {
        perror("Error creando memoria compartida (RAM)");
        return 1;
    }

    // Inicializar RAM a ceros
    char *memoria = (char *) shmat(shmid_mem, NULL, 0);
    if (memoria != (char *)-1) {
        memset(memoria, 0, sharedMemorySize * CELL_SIZE);
        shmdt(memoria);
    }

    // Crear segmento de memoria compartida para la tabla de estado de hilos
    shmid_estado = shmget(KEY_ESTADO, sizeof(TablaEstado), IPC_CREAT | 0666);
    if (shmid_estado == -1) {
        perror("Error creando memoria compartida (Estado)");
        return 1;
    }

    // Inicializar tabla de estado a ceros
    TablaEstado *estado = (TablaEstado *) shmat(shmid_estado, NULL, 0);
    if (estado != (TablaEstado *)-1) {
        memset(estado, 0, sizeof(TablaEstado));
        shmdt(estado);
    }

    // Crear/resetear semáforos nombrados
    sem_unlink(SEM_MEMORIA);
    sem_unlink(SEM_BITACORA);
    sem_unlink(SEM_ESTADO);

    sem_t *s1 = sem_open(SEM_MEMORIA, O_CREAT, 0666, 1);
    sem_t *s2 = sem_open(SEM_BITACORA, O_CREAT, 0666, 1);
    sem_t *s3 = sem_open(SEM_ESTADO, O_CREAT, 0666, 1);

    if (s1 == SEM_FAILED || s2 == SEM_FAILED || s3 == SEM_FAILED) {
        perror("Error creando semaforos");
        return 1;
    }
    sem_close(s1);
    sem_close(s2);
    sem_close(s3);

    // Crear/vaciar archivo de bitácora
    FILE *bitacora = fopen("bitacora.txt", "w");
    if (bitacora) {
        fprintf(bitacora, "=== BITACORA DE EVENTOS ===\n");
        fclose(bitacora);
    }

    // Guardar información en datos.txt
    FILE *archivo = fopen("datos.txt", "w");
    if (archivo != NULL) {
        fprintf(archivo, "Id Memoria Compartida: %d\n", shmid_mem);
        fprintf(archivo, "Id Estado: %d\n", shmid_estado);
        fprintf(archivo, "Tamanno Memoria: %d\n", sharedMemorySize);
        fprintf(archivo, "Producir: true\n");
        fclose(archivo);
    } else {
        printf("Error: No se pudo abrir datos.txt para escribir\n");
    }

    printf("Inicializacion completada.\n");
    printf("  shmid RAM: %d (%d celdas x %d bytes = %d bytes)\n",
           shmid_mem, sharedMemorySize, CELL_SIZE, sharedMemorySize * CELL_SIZE);
    printf("  shmid Estado: %d\n", shmid_estado);
    printf("  Semaforos: %s, %s, %s\n", SEM_MEMORIA, SEM_BITACORA, SEM_ESTADO);
    printf("  Bitacora: bitacora.txt\n");

    return 0;
}