#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main() {
    int shmid = -1;
    int sizeMemoria = 0;

    // 1. Leer shmid y tamaño desde datos.txt
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) {
        printf("Error: No se encontró datos.txt. Ejecuta inicializador primero.\n");
        return 1;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, "Id Memoria Compartida: %d", &shmid);
        sscanf(linea, "Tamanno Memoria: %d", &sizeMemoria);
    }
    fclose(archivo);

    if (shmid == -1 || sizeMemoria == 0) {
        printf("Error: No se pudo extraer shmid o tamaño de datos.txt\n");
        return 1;
    }

    printf("=== ESPIA DE MEMORIA COMPARTIDA ===\n");
    printf("shmid: %d | Celdas: %d | Bytes totales: %d\n\n", shmid, sizeMemoria, sizeMemoria * 4);

    // 2. Adjuntar la memoria compartida en modo solo lectura
    char *memoria = (char *) shmat(shmid, NULL, SHM_RDONLY);
    if (memoria == (char *)-1) {
        perror("Error al adjuntar memoria compartida (shmat)");
        return 1;
    }

    // 3. Recorrer e imprimir cada celda de 4 bytes
    int ocupadas = 0;
    int vacias = 0;

    printf(" Celda | Contenido | Estado\n");
    printf("-------+-----------+--------\n");

    for (int i = 0; i < sizeMemoria; i++) {
        char bloque[5];
        // Copiar los 4 bytes de esa celda a un buffer temporal
        memcpy(bloque, &memoria[i * 4], 4);
        bloque[4] = '\0'; // Asegurar terminación del string

        if (memoria[i * 4] == 0) {
            printf(" %4d  |   ----    | Vacia\n", i);
            vacias++;
        } else {
            printf(" %4d  |   %-6s  | Ocupada\n", i, bloque);
            ocupadas++;
        }
    }

    printf("\n=== RESUMEN ===\n");
    printf("Celdas ocupadas: %d\n", ocupadas);
    printf("Celdas vacias:   %d\n", vacias);
    printf("Total:           %d\n", sizeMemoria);

    // 4. Desconectar la memoria
    shmdt(memoria);

    return 0;
}
