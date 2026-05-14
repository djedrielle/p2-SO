# Proyecto 2 — Simulación de Paginación y Segmentación

Simulación de administración de memoria mediante **Paginación** o **Segmentación** utilizando memoria compartida (System V `shmget`), semáforos nombrados POSIX y múltiples procesos concurrentes en C.

---

## Descripción del Proyecto

Este proyecto simula cómo un sistema operativo asigna y libera memoria a procesos que llegan al estado _Ready_. Cada proceso es representado por un **hilo (thread)** que compite por espacio en una memoria compartida.

El sistema está compuesto por **4 programas independientes** que se comunican entre sí a través de memoria compartida e IPC:

| Programa | Rol |
|---|---|
| `inicializador` | Crea y prepara el ambiente (memoria, semáforos, bitácora) |
| `productor` | Genera hilos que simulan procesos buscando memoria |
| `espia` | Permite observar el estado de la memoria y los procesos en tiempo real |
| `finalizador` | Detiene la simulación y libera todos los recursos del sistema |

### Algoritmos Soportados

- **Paginación**: cada hilo solicita entre 1 y 10 páginas de memoria. Las páginas se asignan de forma no contigua.
- **Segmentación (First-Fit)**: cada hilo solicita entre 1 y 5 segmentos, cada uno de 1 a 3 unidades contiguas de memoria.

### Mecanismos de Sincronización

Se utilizan **3 semáforos nombrados POSIX** para garantizar exclusión mutua:

- `/mutex_memoria` — Protege la búsqueda y liberación de la RAM compartida (solo un hilo a la vez puede buscar espacio).
- `/mutex_bitacora` — Protege las escrituras concurrentes al archivo `bitacora.txt`.
- `/mutex_estado` — Protege la tabla de estados de hilos en memoria compartida.

### Memoria Compartida

- **RAM simulada** (key `1234`): arreglo de celdas de 8 bytes que representa la memoria del sistema.
- **Tabla de estado** (key `5678`): estructura compartida donde cada hilo registra su estado actual (bloqueado, buscando, durmiendo, muerto, terminado), visible para el espía.

---

## Compilación

Requiere `gcc` con soporte para pthreads.

```bash
make
```

Para limpiar los binarios:

```bash
make clean
```

---

## Manual de Usuario

### Paso 1 — Inicializar el ambiente

Ejecuta primero el inicializador. Este programa preguntará cuántas celdas de memoria deseas simular.

```bash
./inicializador
```

**Ejemplo de uso:**
```
Ingrese el tamanno de la memoria compartida: 20
```

Esto crea la memoria compartida, los semáforos y el archivo `bitacora.txt`. Los IDs de los recursos quedan guardados en `datos.txt`.

> ⚠️ **Importante**: Debes correr el inicializador **antes** que cualquier otro programa. Si ya existe memoria del sistema operativo de una sesión anterior, ejecuta primero `./finalizador` para limpiar.

---

### Paso 2 — Iniciar el Productor

En una **nueva terminal**, ejecuta el productor e indica el algoritmo deseado:

```bash
./productor
```

**Ejemplo de uso:**
```
Algoritmo (seg/pag): pag
```

El productor comenzará a crear hilos periódicamente (cada 30–60 segundos de forma aleatoria). Cada hilo buscará espacio en memoria, dormirá si lo consigue, y luego lo liberará.

---

### Paso 3 — Observar con el Espía

Mientras el productor corre, abre **otra terminal** y ejecuta el espía:

```bash
./espia
```

Verás un menú interactivo con las siguientes opciones:

```
--- MENU ESPIA ---
1. Ver estado de la memoria
2. Ver estado de los procesos
3. Ver bitacora
4. Salir
```

- **Opción 1**: Muestra cada celda de memoria (ocupada o vacía) y quién la ocupa.
- **Opción 2**: Lista los hilos clasificados por estado:
  - *En memoria (durmiendo)*: hilos que consiguieron espacio y están en su `sleep`.
  - *Buscando espacio*: el único hilo dentro de la región crítica actualmente.
  - *Bloqueados*: hilos esperando su turno para entrar a buscar.
  - *Muertos*: hilos que no encontraron espacio suficiente.
  - *Terminados*: hilos que completaron su ciclo exitosamente.
- **Opción 3**: Imprime el contenido completo de `bitacora.txt`.

Puedes consultar el espía tantas veces como quieras sin interferir con la simulación.

---

### Paso 4 — Finalizar la simulación

Cuando desees detener todo, ejecuta el finalizador:

```bash
./finalizador
```

Este programa:
1. Cambia la bandera `Producir` a `false` en `datos.txt` (el productor se detendrá en su próxima iteración).
2. Destruye ambos segmentos de memoria compartida.
3. Elimina los 3 semáforos nombrados.
4. Escribe el cierre en `bitacora.txt`.

---

## Bitácora de Eventos

El archivo `bitacora.txt` registra automáticamente cada acción relevante con timestamp:

```
[HH:MM:SS] Hilo ID=3 | Accion: ASIGNACION | Tipo: Paginacion | Paginas: 4 | Celdas: [0,2,5,9]
[HH:MM:SS] Hilo ID=3 | Accion: DESASIGNACION | Tipo: Paginacion | Paginas: 4 | Celdas: [0,2,5,9]
[HH:MM:SS] Hilo ID=7 | Accion: MUERTE | Tipo: Segmentacion | Segmentos: 3 | Necesitaba: 7 unidades
```

---

## Estructura de Archivos

```
p2-SO/
├── comun.h          # Constantes, structs y estados compartidos
├── inicializador.c  # Crea el ambiente
├── productor.c      # Genera los hilos/procesos
├── espia.c          # Observador interactivo
├── finalizador.c    # Limpia y termina la simulación
├── Makefile         # Compilación
├── datos.txt        # Configuración generada por inicializador (auto-generado)
└── bitacora.txt     # Log de eventos (auto-generado)
```