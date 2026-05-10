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
    int sizeMemoriaCompartida;
    int idHilo;
} argumentosPaginacion;
/*
typedef struct {
    int segmentos;
    int unidadXsegmento[segmentos];
    int tiempo;
} argumentosSegmentacion;
*/

void* threadsPaginacion (void* args) {
    argumentosPaginacion* params = (argumentosPaginacion*)args;
    
    // Asignarle un nombre al thread para verlo en la terminal
    char nombreThread[10];
    sprintf(nombreThread, "Thread%d", params->idHilo);
    pthread_setname_np(pthread_self(), nombreThread);

    // Generar el string exacto que vamos a escribir y buscar
    char mi_id_str[10];
    sprintf(mi_id_str, "ID%d", params->idHilo);

    // Entramos a zona crítica para interactuar con la RAM compartida
    sem_wait(sem_mutex);

    int paginasTomadas = 0;
    
    // 1. Buscar espacios vacíos en memoria y apropiárselos
    for (int i = 0; i < params->sizeMemoriaCompartida; i++) {
        if (params->memoriaCompartida[i * 4] == 0) {
            
            // Reclamamos la página escribiendo nuestro ID
            sprintf(&params->memoriaCompartida[i * 4], "ID%d", params->idHilo);
            paginasTomadas++;
            
            // Si ya encontramos todas las páginas que pidió este hilo
            if (paginasTomadas == params->paginas) {
                break; // Rompemos el for y pasamos a la siguiente fase
            }
        }
    }

    // 2. Verificar si logramos tomar todo el cupo exitosamente
    if (paginasTomadas == params->paginas) {
        printf("   [Thread %d] Consiguio sus %d paginas. Durmiendo %d segs...\n", params->idHilo, params->paginas, params->tiempo);
        
        // Exito: Soltamos el candado para dejar que otros procesos lean la memoria mientras dormimos
        sem_post(sem_mutex);
        sleep(params->tiempo); // Simulamos el trabajo del hilo procesando algun dato
        
        // Despertamos e inmediatamente pedimos el candado de nuevo para poder hacer la limpieza de salida
        sem_wait(sem_mutex);
        printf("   [Thread %d] Desperto. Liberando sus espacios de memoria...\n", params->idHilo);
    } else {
        printf("   [Thread %d] Faltaron paginas de RAM (%d conseguidas de %d necesarias). Liberando... \n", 
               params->idHilo, paginasTomadas, params->paginas);
        
        // Aca el thread muere

        // Fracaso: como jamás logramos todo lo querido, omitimos el sleep. Al no habernos ido a 
        // dormir, seguimos conservando el candado sem_mutex original, así que pasamos directo a limpiar.
    }

    // 3. LIMPIEZA / LIBERACION LINEAL (Eficiente y extremadamente segura)
    // Recorremos nuevamente toda la memoria, interactuando únicamente con nuestra propiedad confirmada
    for (int i = 0; i < params->sizeMemoriaCompartida; i++) {
        // Extraemos un fragmento y verificamos que empate textual con nuestro nombre (ej. "ID1" contra "ID1")
        if (strncmp(&params->memoriaCompartida[i * 4], mi_id_str, strlen(mi_id_str)) == 0) {
            // Remplazar el pedazo de string por puros '\0' (ceros), lo que lo deja totalmente "Vacio" de nuevo
            memset(&params->memoriaCompartida[i * 4], 0, 4); 
        }
    }
    
    // Hemos terminado de limpiar. Salimos de zona crítica
    sem_post(sem_mutex);
    
    // Evitar Memory Leaks liberando nuestra struct en el Heap (creada con malloc en productor.c Main)
    free(params);

    return NULL;
}

void* threadsSegmentacion (void* args) {
    sem_wait(sem_mutex);

    // Buscar espacios en memoria (con segmentacion)

    sem_post(sem_mutex);
}

int main() {
    int shmid;
    char algoritmo[10];

    // Inicializar el semáforo ahora sí adentro de la función principal
    sem_mutex = sem_open("/mutex_memoria", O_CREAT, 0666, 1);


    // Extraer el shmid y el tamaño de datos.txt
    FILE *archivo = fopen("datos.txt", "r");
    if (archivo == NULL) {
        printf("Error: Debes correr primero inicializador.c\n");
        return 1;
    }
    
    char linea[256];
    int sizeMemoria = 0;
    shmid = -1; // Lo inicializamos en -1 por si nunca lo encontramos
    
    while (fgets(linea, sizeof(linea), archivo)) {
        // En cada iteración intentamos buscar ambos, ya no usamos break porque ocupamos ambos campos
        sscanf(linea, "Id Memoria Compartida: %d", &shmid);
        sscanf(linea, "Tamanno Memoria: %d", &sizeMemoria);
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
    // (El arreglo ya fue llenado de ceros gracias a 'inicializador.c')

    // Solicitar el tipo de algoritmo al usuario
    printf("Algoritmo (seg/pag): ");
    scanf("%s", algoritmo);
    printf("Algoritmo: %s \n", algoritmo);


    // Randomizar tiempo entre creaciones y tiempo de sleep de thread
    srand(time(NULL));
    int lapsoEntreCreacion = (rand() % 31) + 30;

    if (strcmp(algoritmo, "seg") != 0 && strcmp(algoritmo, "pag") != 0) {
        printf("Ingrese una opcion valida\n");
        return 1;
    }

    int contador_hilos = 1; // Para asignarle un ID bonito a cada thread
    
    printf("Iniciando ciclo productor...\n");

    if (strcmp(algoritmo, "pag") == 0) {
            
            while (1) {
                // 1. Leer el valor de "Producir"
                if (!debeProducir()) {
                    printf("La bandera Producir está en false. Deteniendo productor...\n");
                    break;
                }

                // Importante: El Malloc debe ir ADENTRO del While
                // Si el malloc estuviera afuera, todos los hilos compartirían la misma variable de tiempo y páginas
                // cambiándose los datos entre ellos sorpresivamente (race condition).
                argumentosPaginacion* args = malloc(sizeof(argumentosPaginacion));
                args->memoriaCompartida = memoria_compartida;
                
                // Usamos el tamaño real extraído del .txt:
                args->sizeMemoriaCompartida = sizeMemoria; 
                
                args->paginas = (rand() % 10) + 1;
                args->tiempo = (rand() % 41) + 20;
                
                args->idHilo = contador_hilos;
                contador_hilos++;

                pthread_t hilo_nuevo;
                pthread_create(&hilo_nuevo, NULL, threadsPaginacion, args);
                
                pthread_detach(hilo_nuevo);
                printf("--> Hilo Pag %d (Paginas: %d, Sleep: %d) creado. Esperando %d seg...\n", args->idHilo, args->paginas, args->tiempo, lapsoEntreCreacion);
                sleep(lapsoEntreCreacion);
            }
            
        } else if (strcmp(algoritmo, "seg") == 0) {
            // Analogamente para la función de segmentación
            pthread_t hilo_nuevo;
            pthread_create(&hilo_nuevo, NULL, threadsSegmentacion, NULL);
            pthread_detach(hilo_nuevo);
            
            printf("--> Hilo Seg creado. Esperando %d seg...\n", lapsoEntreCreacion);
        }
    

    return 0;
}