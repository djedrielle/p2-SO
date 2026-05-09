#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdbool.h>

// El semáforo debe ser global para que los hilos tengan acceso a el, pero no puede inicializarse aquí.
sem_t *sem_mutex;

// Función dedicada para leer "datos.txt" repetidamente en el ciclo y ver si debe producir
bool debeProducir() {
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) return false;
    
    char linea[256];
    bool producir = false;
    while (fgets(linea, sizeof(linea), archivo)) {
        char valor[10];
        if (sscanf(linea, "Producir: %s", valor) == 1) {
            if (strcmp(valor, "true") == 0) {
                producir = true;
            }
            break;
        }
    }
    fclose(archivo);
    return producir;
}

typedef struct {
    int paginas;
    int tiempo;
    char* memoriaCompartida;
} argumentosPaginacion;
/*
typedef struct {
    int segmentos;
    int unidadXsegmento[segmentos];
    int tiempo;
} argumentosSegmentacion;
*/

void* producirEnPaginacion (void* args) {
    sem_wait(sem_mutex);

    /* Buscar espacios en memoria (con paginacion)
    Para obtener espacios en memoria aca el mapeo sera el siguiente:
    
    Pagina 1 -> Frames 0-3
    Pagina 2 -> Frame 4-7
    ...
    En la primera llenada todos los frames de un proceso quedaran juntos
    pero conforme se vayan llenando frames comunes se separaran.
    */

    sem_post(sem_mutex);
}

void* producirEnSegmentacion (void* args) {
    sem_wait(sem_mutex);

    // Buscar espacios en memoria (con segmentacion)

    sem_post(sem_mutex);
}

int main() {
    int shmid;
    char algoritmo[10];

    // Inicializar el semáforo ahora sí adentro de la función principal
    sem_mutex = sem_open("/mutex_memoria", O_CREAT, 0666, 1);


    // Extraer el shmid de datos.txt y castear la memoria compartida a un arreglo de caracteres
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) {
        printf("Error: Debes correr primero inicializador.c\n");
        return 1;
    }
    // Buscar el shmid leyendo línea por línea
    char linea[256];
    shmid = -1; // Lo inicializamos en -1 por si nunca lo encontramos
    while (fgets(linea, sizeof(linea), archivo)) {
        if (sscanf(linea, "Id Memoria Compartida: %d", &shmid) == 1) {
            break;
        }
    }
    fclose(archivo);
    if (shmid == -1) {
        printf("Error: No se encontró el 'Id Memoria Compartida' en datos.txt\n");
        return 1;
    }
    // Castear el espacio en memoria compartida a un arreglo de caracteres
    char *memoria_compartida = (char *) shmat(shmid, NULL, 0);
    if (memoria_compartida == (char *)-1) {
        perror("Error apuntando a la memoria (shmat)");
        return 1;
    }

    // Solicitar el tipo de algoritmo al usuario
    printf("Algoritmo (seg/pag): ");
    scanf("%s", algoritmo);
    printf("Algoritmo: %s \n", algoritmo);


    // Randomizar tiempo entre creaciones y tiempo de sleep de thread
    srand(time(NULL));
    int lapsoEntreCreacion = (rand() % 31) + 30;
    int randTiempo = (rand() % 41) + 20;

    if (strcmp(algoritmo, "seg") != 0 && strcmp(algoritmo, "pag") != 0) {
        printf("Ingrese una opcion valida\n");
        return 1;
    }
    
    printf("Iniciando ciclo productor...\n");

    if (strcmp(algoritmo, "pag") == 0) {
            int randPaginas = (rand() % 10) + 1;
            argumentosPaginacion* args = malloc(sizeof(argumentosPaginacion));
            args->paginas = randPaginas;
            args->tiempo = randTiempo;
            args->memoriaCompartida = memoria_compartida;

            while (1) {
                // 1. Leer el valor de "Producir"
                if (!debeProducir()) {
                    printf("La bandera Producir está en false. Deteniendo productor...\n");
                    break;
                }
                pthread_t hilo_nuevo;
                // 2. Crear los hilos dependiendo el tipo instanciado
                pthread_create(&hilo_nuevo, NULL, producirEnPaginacion, args);
                // Usamos detach para que cuando el hilo termine, el OS se encargue solo de limpiar todo
                pthread_detach(hilo_nuevo); 
                // 3. Dormir lo indicado antes de generar el siguiente
                sleep(lapsoEntreCreacion);
            }
            printf("--> Hilo Pag (Paginas: %d, Sleep: %d) creado. Esperando %d seg para el siguiente...\n", args->paginas, args->tiempo, lapsoEntreCreacion);
        } else if (strcmp(algoritmo, "seg") == 0) {
            // Analogamente para la función de segmentación
            pthread_create(&hilo_nuevo, NULL, producirEnSegmentacion, NULL);
            pthread_detach(hilo_nuevo);
            
            printf("--> Hilo Seg creado. Esperando %d seg...\n", lapsoEntreCreacion);
        }
    

    return 0;
}