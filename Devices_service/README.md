Plan de trabajo para el servicio **Devices_service** (MatCom Guard)
===========================================

Este documento sirve como guía paso a paso para la implementación del demonio modular en C, basado en hilos y sockets UNIX, con JSON como protocolo de comunicación entre módulos.

---

## Fase 0: Preparación del entorno

**Tareas:**
1. Crear la estructura de directorios:
   ```
   Devices_service/
   ├── include/
   ├── src/
   ├── build/
   ├── tests/
   ├── Makefile
   └── README.md
   ```
2. Inicializar repositorio Git en `Devices_service/`.
3. Añadir `.gitignore` para ignorar `build/` y binarios.

**Entregable:** Scaffold de proyecto con Makefile funcionando (`make` → genera ejecutable vacío).

---

## Fase 1: Módulo **Watcher**

**Objetivo:** detectar nuevos dispositivos montados.

**Tareas:**
1. Definir API en `include/watcher.h`:
   ```c
   void *watcher_thread(void *arg);
   ```
2. Implementar `src/watcher.c` usando **libudev**:
   - Inicializar contexto Udev.
   - Filtrar eventos `block`/`partition`.
   - Detectar `add` y encolar `DeviceEvent`.
3. Crear pruebas unitarias `tests/test_watcher.c` (simular eventos o refactorizar para modo test).
4. Verificar que `build/watcher` emite eventos correctamente.

**Entregable:** Ejecutable `build/watcher` que detecta y reporta dispositivos.

---

## Fase 2: Módulo **Scanner**

**Objetivo:**, a partir de un `DeviceEvent`, crear snapshots y detectar cambios.

**Tareas:**
1. Definir estructuras en `include/snapshot.h` y `include/diff.h`:
   ```c
   typedef struct Snapshot Snapshot;
   typedef struct FileEvent { enum { ADDED, REMOVED, MODIFIED } type; char path[256]; time_t timestamp; } FileEvent;
   ```
2. Implementar `src/snapshot.c`:
   - `Snapshot *snapshot_create(const char *mount_point);`
   - `void snapshot_free(Snapshot *s);`
3. Implementar `src/diff.c`:
   - `List *snapshot_diff(Snapshot *old, Snapshot *new);`
4. Programar `src/scanner.c`:
   - `void *scanner_thread(void *arg);`
   - Esperar en cola, generar snapshots, comparar y encolar `FileEvent`.
5. Pruebas en `tests/test_scanner.c` (directorios de prueba, modificaciones).

**Entregable:** Ejecutable `build/scanner` que detecta cambios de archivos.

---

## Fase 3: Módulo **Alerter**

**Objetivo:** consumir `FileEvent`, aplicar filtros y generar alertas/logs.

**Tareas:**
1. Definir API en `include/alerter.h`:
   ```c
   void *alerter_thread(void *arg);
   ```
2. Implementar `src/alerter.c`:
   - Colas protegidas con mutex y condvars.
   - Filtrado por extensión/tamaño.
   - Logging con `syslog()` o archivo plano.
   - Emisión de JSON de alerta.
3. Pruebas en `tests/test_alerter.c`.

**Entregable:** Ejecutable `build/alerter` que procesa eventos y alerta.

---

## Fase 4: Módulo **Socket Server**

**Objetivo:** exponer estado vía socket UNIX para clientes externos.

**Tareas:**
1. Definir API en `include/socket_srv.h`:
   ```c
   void *socket_server_thread(void *arg);
   ```
2. Implementar `src/socket_srv.c`:
   - Crear socket AF_UNIX (`/tmp/matcom_guard.sock`).
   - `bind()`, `listen()`, `accept()`.
   - Protocolo: cliente envía `STATUS\n`, servidor responde JSON y cierra.
   - (Opcional) `fork()` para cada conexión.
3. Pruebas en `tests/test_socket_srv.c` (con `nc -U`).

**Entregable:** Ejecutable `build/socket_srv` escuchando en el socket.

---

## Fase 5: Orquestación en **Main**

**Objetivo:** ensamblar hilos y coordinar demonio.

**Tareas:**
1. Programar `src/main.c`:
   - Parsear argumentos y cargar config.
   - Inicializar colas (`pthread_mutex_init`, `pthread_cond_init`).
   - Lanzar hilos: `watcher_thread`, `scanner_thread`, `alerter_thread`, `socket_server_thread`.
   - Manejar señales (`SIGINT`/`SIGTERM`) para shutdown ordenado.
2. Actualizar Makefile:
   - Regla para compilar cada módulo y el demonio final.
   - Opcional: target `install`.

**Entregable:** Ejecutable `devices_service` que arranca todo como demonio.

---

## Fase 6: Pruebas de integración y documentación

**Tareas:**
1. Script end-to-end:
   - Montaje/desmontaje simulado (`mount --bind`).
   - Verificar logs, JSON, socket.
2. Documentar en `README.md`:
   - Descripción de módulos e interfaces JSON.
   - Instrucciones de compilación, instalación y ejecución.
   - Ejemplos de uso y pruebas.
3. Revisión de código:
   - Valgrind para fugas de memoria.
   - Manejo de errores sólido.

**Entregable:** Documentación completa y pruebas automatizadas.

---

## Cronograma sugerido

| Semana | Fase               | Objetivo                                |
|:------:|--------------------|-----------------------------------------|
| 1      | 0 + 1              | Preparación + Watcher                   |
| 2      | 2                  | Scanner                                 |
| 3      | 3                  | Alerter                                 |
| 4      | 4                  | Socket Server                           |
| 5      | 5                  | Integración y demonio                   |
| 6      | 6                  | Pruebas finales y documentación         |

---

Con este plan modular, cada integrante puede trabajar en paralelo en un módulo específico, respetando las interfaces definidas y facilitando la integración final.


