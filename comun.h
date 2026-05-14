#ifndef COMUN_H
#define COMUN_H

#define KEY_MEMORIA 1234
#define KEY_ESTADO  5678
#define CELL_SIZE   8
#define MAX_HILOS   100

#define SEM_MEMORIA  "/mutex_memoria"
#define SEM_BITACORA "/mutex_bitacora"
#define SEM_ESTADO   "/mutex_estado"

// Estados de los hilos
#define ESTADO_LIBRE      0
#define ESTADO_BLOQUEADO  1
#define ESTADO_BUSCANDO   2
#define ESTADO_DURMIENDO  3
#define ESTADO_MUERTO     4
#define ESTADO_TERMINADO  5

typedef struct {
    int idHilo;
    int estado;
    int unidades;   // total de unidades de memoria asignadas
    int tiempo;     // tiempo de sleep
} EstadoHilo;

typedef struct {
    int totalHilos;
    EstadoHilo hilos[MAX_HILOS];
} TablaEstado;

#endif
