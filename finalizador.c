// Programa Finalizador
// Detiene la producción, destruye memoria compartida y semáforos, cierra bitácora.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include "comun.h"

int main() {
    int shmid = -1, shmid_estado = -1;
    int sizeMemoria = 0;

    // 1. Leer datos.txt para obtener los IDs
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) {
        printf("Error: No se encontro datos.txt\n");
        return 1;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, "Id Memoria Compartida: %d", &shmid);
        sscanf(linea, "Id Estado: %d", &shmid_estado);
        sscanf(linea, "Tamanno Memoria: %d", &sizeMemoria);
    }
    fclose(archivo);

    // 2. Cambiar la bandera Producir a false para indicarle al programa productor que deje de producir hilos
    archivo = fopen("datos.txt", "w");
    if (archivo != NULL) {
        fprintf(archivo, "Id Memoria Compartida: %d\n", shmid);
        fprintf(archivo, "Id Estado: %d\n", shmid_estado);
        fprintf(archivo, "Tamanno Memoria: %d\n", sizeMemoria);
        fprintf(archivo, "Producir: false\n");
        fclose(archivo);
        printf("[OK] Bandera 'Producir' cambiada a false.\n");
    }

    // 3. Escribir cierre en la bitácora
    FILE *bitacora = fopen("bitacora.txt", "a");
    if (bitacora) {
        fprintf(bitacora, "\n=== FINALIZADOR EJECUTADO ===\n");
        fprintf(bitacora, "Recursos liberados. Fin de la simulacion.\n");
        fclose(bitacora);
        printf("[OK] Bitacora cerrada.\n");
    }

    // 4. Destruir segmentos de memoria compartida
    if (shmid != -1) {
        if (shmctl(shmid, IPC_RMID, NULL) == 0)
            printf("[OK] Memoria compartida RAM (shmid=%d) eliminada.\n", shmid);
        else
            perror("[WARN] No se pudo eliminar memoria RAM");
    }

    if (shmid_estado != -1) {
        if (shmctl(shmid_estado, IPC_RMID, NULL) == 0)
            printf("[OK] Memoria compartida Estado (shmid=%d) eliminada.\n", shmid_estado);
        else
            perror("[WARN] No se pudo eliminar memoria Estado");
    }

    // 5. Eliminar semáforos nombrados
    if (sem_unlink(SEM_MEMORIA) == 0)
        printf("[OK] Semaforo %s eliminado.\n", SEM_MEMORIA);
    else
        perror("[WARN] sem_unlink mutex_memoria");

    if (sem_unlink(SEM_BITACORA) == 0)
        printf("[OK] Semaforo %s eliminado.\n", SEM_BITACORA);
    else
        perror("[WARN] sem_unlink mutex_bitacora");

    if (sem_unlink(SEM_ESTADO) == 0)
        printf("[OK] Semaforo %s eliminado.\n", SEM_ESTADO);
    else
        perror("[WARN] sem_unlink mutex_estado");

    printf("\nFinalizacion completada. Todos los recursos han sido liberados.\n");
    printf("Si el productor aun esta corriendo, se detendra en su proxima iteracion.\n");

    return 0;
}
