#define _GNU_SOURCE
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
#include <stdarg.h>
#include "comun.h"

// Semaforos
sem_t *sem_mutex;
sem_t *sem_bitacora;
sem_t *sem_estado;

// Tabla de estado global (memoria compartida)
TablaEstado *tablaEstado;

// ===================== FUNCIONES AUXILIARES =====================

// Esta funcion permite que procesos externos controlen si el programa productor
// puede o no seguir produciendo
bool debeProducir() {
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) return false;
    char linea[256];
    bool producir = false;
    while (fgets(linea, sizeof(linea), archivo)) {
        char valor[10];
        if (sscanf(linea, "Producir: %s", valor) == 1) {
            if (strcmp(valor, "true") == 0) producir = true;
            break;
        }
    }
    fclose(archivo);
    return producir;
}

void escribirBitacora(const char* formato, ...) {
    sem_wait(sem_bitacora);
    FILE *f = fopen("bitacora.txt", "a");
    if (f) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        fprintf(f, "[%02d:%02d:%02d] ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        va_list args;
        va_start(args, formato);
        vfprintf(f, formato, args);
        va_end(args);
        fprintf(f, "\n");
        fclose(f);
    }
    sem_post(sem_bitacora);
}

void registrarHilo(int idHilo, int unidades, int tiempo) { // Registrar hilo en la tabla de estados
    sem_wait(sem_estado);
    int idx = tablaEstado->totalHilos;
    if (idx < MAX_HILOS) {
        tablaEstado->hilos[idx].idHilo = idHilo;
        tablaEstado->hilos[idx].estado = ESTADO_BLOQUEADO;
        tablaEstado->hilos[idx].unidades = unidades;
        tablaEstado->hilos[idx].tiempo = tiempo;
        tablaEstado->totalHilos++;
    }
    sem_post(sem_estado);
}

void actualizarEstado(int idHilo, int nuevoEstado) {
    for (int i = 0; i < tablaEstado->totalHilos; i++) {
        if (tablaEstado->hilos[i].idHilo == idHilo) {
            tablaEstado->hilos[i].estado = nuevoEstado;
            break;
        }
    }
}

// Construye un string con la lista de celdas, ej: "[0,3,7]"
void construirListaCeldas(int *celdas, int n, char *buffer, int bufSize) {
    strcpy(buffer, "[");
    for (int i = 0; i < n; i++) {
        char tmp[16];
        sprintf(tmp, "%d%s", celdas[i], (i < n - 1) ? "," : "");
        strcat(buffer, tmp);
    }
    strcat(buffer, "]");
}

// ===================== STRUCTS DE ARGUMENTOS =====================

typedef struct {
    int paginas;
    int tiempo;
    char* memoriaCompartida;
    int sizeMemoriaCompartida;
    int idHilo;
} argumentosPaginacion;

typedef struct {
    int numSegmentos;
    int tamSegmento[5];   // cada segmento tiene 1-3 unidades
    int tiempo;
    char* memoriaCompartida;
    int sizeMemoriaCompartida;
    int idHilo;
} argumentosSegmentacion;

// ===================== THREAD PAGINACIÓN =====================

void* threadsPaginacion(void* args) { // Se recibe el struct argumentosPaginacion como parametro
    argumentosPaginacion* params = (argumentosPaginacion*)args;

    char nombreThread[16];
    sprintf(nombreThread, "Thread%d", params->idHilo);
    pthread_setname_np(pthread_self(), nombreThread);

    char mi_id_str[CELL_SIZE];
    sprintf(mi_id_str, "ID%d", params->idHilo);

    registrarHilo(params->idHilo, params->paginas, params->tiempo); // Registrar el hilo en la tabla de estados

    // Pedir semáforo de memoria
    actualizarEstado(params->idHilo, ESTADO_BLOQUEADO);
    sem_wait(sem_mutex);
    actualizarEstado(params->idHilo, ESTADO_BUSCANDO);

    // Buscar ubicación
    int paginasTomadas = 0;
    int celdasAsignadas[10];

    // Bucle encargado de la busqueda de celdas libres en memoria
    for (int i = 0; i < params->sizeMemoriaCompartida; i++) {
        if (params->memoriaCompartida[i * CELL_SIZE] == 0) {
            celdasAsignadas[paginasTomadas] = i;
            sprintf(&params->memoriaCompartida[i * CELL_SIZE], "ID%d", params->idHilo);
            paginasTomadas++;
            if (paginasTomadas == params->paginas) break; // Si el proceso ya tomo todas las paginas que buscaba entonces se deja de buscar
        }
    }

    if (paginasTomadas == params->paginas) { // Si encontramos todas las paginas que buscabamos, entonces...
        // Indicar en bitacora que se encontro campo en memoria
        char celdasStr[128];
        construirListaCeldas(celdasAsignadas, paginasTomadas, celdasStr, sizeof(celdasStr));
        escribirBitacora("Hilo ID=%d | Accion: ASIGNACION | Tipo: Paginacion | Paginas: %d | Celdas: %s",
                         params->idHilo, params->paginas, celdasStr);

        printf("   [Thread %d] Consiguio sus %d paginas. Durmiendo %d segs...\n",
               params->idHilo, params->paginas, params->tiempo);

        // Se libera el semaforo de memoria
        actualizarEstado(params->idHilo, ESTADO_DURMIENDO);
        sem_post(sem_mutex);

        // Sleep
        sleep(params->tiempo);

        // Pedir semáforo de memoria
        actualizarEstado(params->idHilo, ESTADO_BLOQUEADO);
        sem_wait(sem_mutex);
        actualizarEstado(params->idHilo, ESTADO_BUSCANDO);
        printf("   [Thread %d] Desperto. Liberando sus espacios de memoria...\n", params->idHilo);

        // Liberar memoria
        for (int i = 0; i < params->sizeMemoriaCompartida; i++) {
            if (strncmp(&params->memoriaCompartida[i * CELL_SIZE], mi_id_str, strlen(mi_id_str)) == 0) {
                memset(&params->memoriaCompartida[i * CELL_SIZE], 0, CELL_SIZE);
            }
        }

        // Indicar en bitacora que se libera la memoria tomada
        escribirBitacora("Hilo ID=%d | Accion: DESASIGNACION | Tipo: Paginacion | Paginas: %d | Celdas: %s",
                         params->idHilo, params->paginas, celdasStr);
        actualizarEstado(params->idHilo, ESTADO_TERMINADO);

    } else {
        // En caso de no haber encontrado espacio suficiente, el hilo muere
        printf("   [Thread %d] Faltaron paginas (%d/%d). Muriendo...\n",
               params->idHilo, paginasTomadas, params->paginas);

        // Limpiar las parciales que sí tomó
        for (int i = 0; i < params->sizeMemoriaCompartida; i++) {
            if (strncmp(&params->memoriaCompartida[i * CELL_SIZE], mi_id_str, strlen(mi_id_str)) == 0) {
                memset(&params->memoriaCompartida[i * CELL_SIZE], 0, CELL_SIZE);
            }
        }

        escribirBitacora("Hilo ID=%d | Accion: MUERTE | Tipo: Paginacion | Necesitaba: %d | Encontro: %d",
                         params->idHilo, params->paginas, paginasTomadas);
        actualizarEstado(params->idHilo, ESTADO_MUERTO);
    }

    // Devolver semáforo de memoria
    sem_post(sem_mutex);
    free(params);
    return NULL;
}

// ===================== THREAD SEGMENTACIÓN =====================

void* threadsSegmentacion(void* args) {
    argumentosSegmentacion* params = (argumentosSegmentacion*)args;

    char nombreThread[16];
    sprintf(nombreThread, "Thread%d", params->idHilo);
    pthread_setname_np(pthread_self(), nombreThread);

    char mi_id_str[CELL_SIZE];
    sprintf(mi_id_str, "ID%d", params->idHilo);

    int totalNecesarias = 0; // Cantidad de segmentos que busca el hilo
    for (int s = 0; s < params->numSegmentos; s++)
        totalNecesarias += params->tamSegmento[s];

    registrarHilo(params->idHilo, totalNecesarias, params->tiempo); // Se registra el hilo

    // Se pide el semaforo para empezar a buscar espacios en memoria
    actualizarEstado(params->idHilo, ESTADO_BLOQUEADO);
    sem_wait(sem_mutex);
    actualizarEstado(params->idHilo, ESTADO_BUSCANDO);

    // Se busca espacio en memoria (con First-Fit)
    int celdasAsignadas[15]; // max 5 segmentos * 3 unidades
    int totalAsignadas = 0;
    bool exito = true;

    // Bucle encargado de buscar en memoria
    for (int s = 0; s < params->numSegmentos && exito; s++) {
        int tamSeg = params->tamSegmento[s]; // Guardamos el tamano del segmento que buscamos
        bool encontrado = false;
        // Para cada segmento hay que recorrer la memoria en busca de espacio
        for (int i = 0; i <= params->sizeMemoriaCompartida - tamSeg; i++) {
            bool bloqueLibre = true;
            for (int j = 0; j < tamSeg; j++) {
                // Si el bloque de memoria es del tamano del segmento y esta libre, entonces se encontro espacio
                if (params->memoriaCompartida[(i + j) * CELL_SIZE] != 0) {
                    bloqueLibre = false;
                    break;
                }
            }
            if (bloqueLibre) {
                for (int j = 0; j < tamSeg; j++) {
                    sprintf(&params->memoriaCompartida[(i + j) * CELL_SIZE], "ID%d", params->idHilo);
                    celdasAsignadas[totalAsignadas++] = i + j;
                }
                encontrado = true;
                break;
            }
        }
        if (!encontrado) exito = false;
    }

    if (exito) {
        // Si se encontro memoria disponible, indicarlo en la bitacora
        char celdasStr[128];
        construirListaCeldas(celdasAsignadas, totalAsignadas, celdasStr, sizeof(celdasStr));
        escribirBitacora("Hilo ID=%d | Accion: ASIGNACION | Tipo: Segmentacion | Segmentos: %d | Unidades: %d | Celdas: %s",
                         params->idHilo, params->numSegmentos, totalAsignadas, celdasStr);

        printf("   [Thread %d] Consiguio %d segmentos (%d unidades). Durmiendo %d segs...\n",
               params->idHilo, params->numSegmentos, totalAsignadas, params->tiempo);

        // Devolver semáforo
        actualizarEstado(params->idHilo, ESTADO_DURMIENDO);
        sem_post(sem_mutex);

        // Sleep
        sleep(params->tiempo);

        // Pedir semáforo
        actualizarEstado(params->idHilo, ESTADO_BLOQUEADO);
        sem_wait(sem_mutex);
        actualizarEstado(params->idHilo, ESTADO_BUSCANDO);
        printf("   [Thread %d] Desperto. Liberando memoria...\n", params->idHilo);

        // Liberar memoria
        for (int i = 0; i < params->sizeMemoriaCompartida; i++) {
            if (strncmp(&params->memoriaCompartida[i * CELL_SIZE], mi_id_str, strlen(mi_id_str)) == 0)
                memset(&params->memoriaCompartida[i * CELL_SIZE], 0, CELL_SIZE);
        }

        // Escribir en Bitácora (desasignación)
        escribirBitacora("Hilo ID=%d | Accion: DESASIGNACION | Tipo: Segmentacion | Celdas: %s",
                         params->idHilo, celdasStr);
        actualizarEstado(params->idHilo, ESTADO_TERMINADO);

    } else {
        printf("   [Thread %d] Sin espacio contiguo para sus segmentos. Muriendo...\n", params->idHilo);

        // Limpiar parciales
        for (int i = 0; i < params->sizeMemoriaCompartida; i++) {
            if (strncmp(&params->memoriaCompartida[i * CELL_SIZE], mi_id_str, strlen(mi_id_str)) == 0)
                memset(&params->memoriaCompartida[i * CELL_SIZE], 0, CELL_SIZE);
        }

        escribirBitacora("Hilo ID=%d | Accion: MUERTE | Tipo: Segmentacion | Segmentos: %d | Necesitaba: %d unidades",
                         params->idHilo, params->numSegmentos, totalNecesarias);
        actualizarEstado(params->idHilo, ESTADO_MUERTO);
    }

    // Devolver semáforo
    sem_post(sem_mutex);
    free(params);
    return NULL;
}

// ===================== MAIN =====================

int main() {
    int shmid, shmid_estado;
    char algoritmo[10];

    // Abrir semáforos
    sem_mutex    = sem_open(SEM_MEMORIA,  O_CREAT, 0666, 1);
    sem_bitacora = sem_open(SEM_BITACORA, O_CREAT, 0666, 1);
    sem_estado   = sem_open(SEM_ESTADO,   O_CREAT, 0666, 1);

    if (sem_mutex == SEM_FAILED || sem_bitacora == SEM_FAILED || sem_estado == SEM_FAILED) {
        perror("Error abriendo semaforos");
        return 1;
    }

    // Leer datos.txt
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) {
        printf("Error: Debes correr primero inicializador\n");
        return 1;
    }

    char linea[256];
    int sizeMemoria = 0;
    shmid = -1;
    shmid_estado = -1;

    while (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, "Id Memoria Compartida: %d", &shmid);
        sscanf(linea, "Id Estado: %d", &shmid_estado);
        sscanf(linea, "Tamanno Memoria: %d", &sizeMemoria);
    }
    fclose(archivo);

    if (shmid == -1 || shmid_estado == -1) {
        printf("Error: No se encontraron IDs de memoria en datos.txt\n");
        return 1;
    }

    // Adjuntar memorias compartidas
    char *memoria_compartida = (char *) shmat(shmid, NULL, 0);
    if (memoria_compartida == (char *)-1) {
        perror("Error apuntando a la memoria (shmat)");
        return 1;
    }

    tablaEstado = (TablaEstado *) shmat(shmid_estado, NULL, 0);
    if (tablaEstado == (TablaEstado *)-1) {
        perror("Error apuntando a la tabla de estado (shmat)");
        return 1;
    }

    // Solicitar algoritmo al usuario
    printf("Algoritmo (seg/pag): ");
    scanf("%s", algoritmo);
    printf("Algoritmo: %s \n", algoritmo);

    srand(time(NULL));

    if (strcmp(algoritmo, "seg") != 0 && strcmp(algoritmo, "pag") != 0) {
        printf("Ingrese una opcion valida\n");
        return 1;
    }

    int contador_hilos = 1;
    printf("Iniciando ciclo productor...\n");

    if (strcmp(algoritmo, "pag") == 0) {
        while (1) {
            if (!debeProducir()) {
                printf("La bandera Producir esta en false. Deteniendo productor...\n");
                break;
            }

            argumentosPaginacion* args = malloc(sizeof(argumentosPaginacion));
            args->memoriaCompartida = memoria_compartida;
            args->sizeMemoriaCompartida = sizeMemoria;
            args->paginas = (rand() % 10) + 1;
            args->tiempo  = (rand() % 41) + 20;
            args->idHilo  = contador_hilos++;

            pthread_t hilo_nuevo;
            pthread_create(&hilo_nuevo, NULL, threadsPaginacion, args);
            pthread_detach(hilo_nuevo);

            int lapsoEntreCreacion = (rand() % 31) + 30;
            printf("--> Hilo Pag %d (Paginas: %d, Sleep: %d) creado. Esperando %d seg...\n",
                   args->idHilo, args->paginas, args->tiempo, lapsoEntreCreacion);
            sleep(lapsoEntreCreacion);
        }

    } else if (strcmp(algoritmo, "seg") == 0) {
        while (1) {
            if (!debeProducir()) {
                printf("La bandera Producir esta en false. Deteniendo productor...\n");
                break;
            }

            argumentosSegmentacion* args = malloc(sizeof(argumentosSegmentacion));
            args->memoriaCompartida = memoria_compartida;
            args->sizeMemoriaCompartida = sizeMemoria;
            args->numSegmentos = (rand() % 5) + 1;
            for (int s = 0; s < args->numSegmentos; s++)
                args->tamSegmento[s] = (rand() % 3) + 1;
            args->tiempo = (rand() % 41) + 20;
            args->idHilo = contador_hilos++;

            pthread_t hilo_nuevo;
            pthread_create(&hilo_nuevo, NULL, threadsSegmentacion, args);
            pthread_detach(hilo_nuevo);

            // Descripción de segmentos para la terminal
            char segDesc[64] = "[";
            for (int s = 0; s < args->numSegmentos; s++) {
                char tmp[8];
                sprintf(tmp, "%d%s", args->tamSegmento[s], (s < args->numSegmentos - 1) ? "," : "");
                strcat(segDesc, tmp);
            }
            strcat(segDesc, "]");

            int lapsoEntreCreacion = (rand() % 31) + 30;
            printf("--> Hilo Seg %d (Segs: %d %s, Sleep: %d) creado. Esperando %d seg...\n",
                   args->idHilo, args->numSegmentos, segDesc, args->tiempo, lapsoEntreCreacion);
            sleep(lapsoEntreCreacion);
        }
    }

    // Cleanup al salir
    shmdt(memoria_compartida);
    shmdt(tablaEstado);
    sem_close(sem_mutex);
    sem_close(sem_bitacora);
    sem_close(sem_estado);

    return 0;
}