#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    int paginas;
    int tiempo;
} argumentosPaginacion;

typedef struct {
    int segmentos;
    int unidadXsegmento;
    int tiempo;
} argumentosSegmentacion;

int main() {
    char algoritmo[10];
    // Solicitar el tipo de algoritmo al usuario
    printf("Algoritmo (seg/pag): ");
    scanf("%s", algoritmo);
    printf("Algoritmo: %s \n", algoritmo);

    if (strcmp(algoritmo, "seg") == 0) {
        printf("Segmentacion\n");
    } else if (strcmp(algoritmo, "pag") == 0) {
        printf("Paginacion\n");
    } else {
        printf("Ingrese una opcion valida\n");
    }



    return 0;
}