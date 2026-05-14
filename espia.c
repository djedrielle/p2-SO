// Programa Espía
// Menú interactivo: estado de memoria, estado de procesos, bitácora.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "comun.h"

void mostrarMemoria(char *memoria, int sizeMemoria) {
    int ocupadas = 0, vacias = 0;

    printf("\n Celda | Contenido   | Estado\n");
    printf("-------+-------------+--------\n");

    for (int i = 0; i < sizeMemoria; i++) {
        char bloque[CELL_SIZE + 1];
        memcpy(bloque, &memoria[i * CELL_SIZE], CELL_SIZE);
        bloque[CELL_SIZE] = '\0';

        if (memoria[i * CELL_SIZE] == 0) {
            printf(" %4d  |   ------    | Vacia\n", i);
            vacias++;
        } else {
            printf(" %4d  |   %-8s  | Ocupada\n", i, bloque);
            ocupadas++;
        }
    }

    printf("\n=== RESUMEN ===\n");
    printf("Celdas ocupadas: %d\n", ocupadas);
    printf("Celdas vacias:   %d\n", vacias);
    printf("Total:           %d\n", sizeMemoria);
}

void mostrarProcesos(TablaEstado *estado) {
    printf("\n=== ESTADO DE PROCESOS ===\n");

    if (estado->totalHilos == 0) {
        printf("No hay hilos registrados.\n");
        return;
    }

    int estados[] = { ESTADO_DURMIENDO, ESTADO_BUSCANDO, ESTADO_BLOQUEADO, ESTADO_MUERTO, ESTADO_TERMINADO };
    const char *titulos[] = {
        "-- En memoria (durmiendo) --",
        "-- Buscando espacio --",
        "-- Bloqueados (esperando semaforo) --",
        "-- Muertos (sin espacio) --",
        "-- Terminados --"
    };

    for (int e = 0; e < 5; e++) {
        printf("\n%s\n", titulos[e]);
        int count = 0;
        for (int i = 0; i < estado->totalHilos; i++) {
            if (estado->hilos[i].estado == estados[e]) {
                printf("  Hilo ID=%d | Unidades: %d | Tiempo: %ds\n",
                       estado->hilos[i].idHilo, estado->hilos[i].unidades, estado->hilos[i].tiempo);
                count++;
            }
        }
        if (count == 0) printf("  (ninguno)\n");
    }

    printf("\nTotal hilos registrados: %d\n", estado->totalHilos);
}

void mostrarBitacora() {
    printf("\n=== BITACORA ===\n");
    FILE *f = fopen("bitacora.txt", "r");
    if (f == NULL) {
        printf("No se encontro bitacora.txt\n");
        return;
    }
    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        printf("%s", linea);
    }
    fclose(f);
}

int main() {
    int shmid = -1, shmid_estado = -1;
    int sizeMemoria = 0;

    // Leer datos.txt
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) {
        printf("Error: No se encontro datos.txt. Ejecuta inicializador primero.\n");
        return 1;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, "Id Memoria Compartida: %d", &shmid);
        sscanf(linea, "Id Estado: %d", &shmid_estado);
        sscanf(linea, "Tamanno Memoria: %d", &sizeMemoria);
    }
    fclose(archivo);

    if (shmid == -1 || shmid_estado == -1 || sizeMemoria == 0) {
        printf("Error: datos.txt incompleto\n");
        return 1;
    }

    // Adjuntar memorias compartidas (solo lectura)
    char *memoria = (char *) shmat(shmid, NULL, SHM_RDONLY);
    if (memoria == (char *)-1) {
        perror("Error al adjuntar memoria RAM (shmat)");
        return 1;
    }

    TablaEstado *estado = (TablaEstado *) shmat(shmid_estado, NULL, SHM_RDONLY);
    if (estado == (TablaEstado *)-1) {
        perror("Error al adjuntar tabla de estado (shmat)");
        return 1;
    }

    printf("=== ESPIA DE MEMORIA COMPARTIDA ===\n");
    printf("shmid RAM: %d | shmid Estado: %d | Celdas: %d | Bytes: %d\n\n",
           shmid, shmid_estado, sizeMemoria, sizeMemoria * CELL_SIZE);

    // Menú interactivo
    int opcion = 0;
    while (opcion != 4) {
        printf("\n--- MENU ESPIA ---\n");
        printf("1. Ver estado de la memoria\n");
        printf("2. Ver estado de los procesos\n");
        printf("3. Ver bitacora\n");
        printf("4. Salir\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1: mostrarMemoria(memoria, sizeMemoria); break;
            case 2: mostrarProcesos(estado); break;
            case 3: mostrarBitacora(); break;
            case 4: printf("Saliendo del espia...\n"); break;
            default: printf("Opcion invalida.\n"); break;
        }
    }

    shmdt(memoria);
    shmdt((void*)estado);
    return 0;
}
