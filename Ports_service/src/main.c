#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glib.h>

#include "port_utils.h"
#include "port_scanner.h"

int main(void) {
    pid_t pid_test = fork();
    if (pid_test < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    else if (pid_test == 0) {
        // Proceso hijo: reemplaza su imagen por el servidor de prueba.
        // Asume que 'test_scanner' ya está compilado y en el mismo directorio.
        execl("./test_scanner", "test_scanner", NULL);
        // Si execl falla:
        perror("execl(\"./test_scanner\")");
        exit(EXIT_FAILURE);
    }

    // Proceso padre:
    // 1) Espera un breve momento para que todos los puertos de prueba queden listening.
    sleep(1);

    // 2) Inicia el escáner de puertos (Tema 3)
    GHashTable *tabla = inicializar_tabla_puertos();
    if (tabla == NULL) {
        fprintf(stderr, "ERROR: No se pudo inicializar la tabla de puertos.\n");
        // Intentamos matar al hijo de test_scanner antes de salir
        kill(pid_test, SIGTERM);
        return EXIT_FAILURE;
    }

    printf("🔍 Escaneando puertos 1–1024 con %d hilos...\n\n", NUM_HILOS);
    escanear_puertos(tabla, 1, 1024);

    g_hash_table_destroy(tabla);

    // 3) Cuando termina el escaneo, esperamos a que el test_scanner acabe (o lo terminamos).
    // test_scanner duerme 20s antes de cerrar todos los puertos. Si el escaneo dura más, 
    // el hijo aún está vivo, por lo que esperamos que termine o forzamos su cierre.
    int status;
    if (waitpid(pid_test, &status, WNOHANG) == 0) {
        // Aún está vivo: 
        kill(pid_test, SIGTERM);
        waitpid(pid_test, &status, 0);
    }

    return EXIT_SUCCESS;
}

/*
  -------------------- SALIDA ESPERADA DEL ESCÁNER --------------------

  Dado que test_scanner crea puertos con estos banners:

    • 21  → "220 Servicio FTP listo"      
    • 22  → "SSH-2.0-OpenSSH_7.9"          
    • 25  → "220 Servicio SMTP listo"     
    • 443 → ""                             
    • 31337 → "HTTP/1.0 200 OK..."        
    • 4444 → "HTTP/1.0 200 OK..."         

  El escáner debería imprimir, para cada puerto abierto:

    Puerto 21   → Servicio: FTP   (esperado) ✅
    Puerto 22   → Servicio: SSH   (esperado) ✅
    Puerto 25   → Servicio: SMTP  (esperado) ✅
    Puerto 443  → Servicio: HTTPS (esperado) ✅

    ⚠️ Puerto 31337 → DESCONOCIDO (no en lista oficial)
        ↪ Banner recibido: Servidor HTTP en puerto no estándar 
        ⚠️ Alerta: Servidor HTTP detectado en puerto no estándar 31337/tcp

    ⚠️ Puerto 4444  → DESCONOCIDO (no en lista oficial)
        ↪ Banner recibido: Servidor HTTP en puerto no estándar 
        ⚠️ Alerta: Servidor HTTP detectado en puerto no estándar 4444/tcp

  – El resto de puertos (cerrados) no imprime nada.
  – Si quisieras ver “Puerto X cerrado” habría que modificar el escáner, 
    pero en la implementación actual los puertos cerrados se silencian.
*/
