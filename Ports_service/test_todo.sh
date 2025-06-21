#!/usr/bin/env bash
echo "🧪 Iniciando todos los servicios/nc listeners de prueba..."

# ----- 1) Servidor HTTP en 8080 (puerto estándar de prueba) -----
python3 -m http.server 8080 --bind 127.0.0.1 >/dev/null 2>&1 &
PID_HTTP_8080=$!
echo "    - HTTP real en 8080 (pid $PID_HTTP_8080)"

# ----- 2) netcat en 31337 (puerto no en tabla) -----
nc -l 127.0.0.1 31337 >/dev/null 2>&1 &
PID_NC_31337=$!
echo "    - netcat en 31337 (pid $PID_NC_31337)"

# ----- 3) netcat en 25 (SMTP simulado, sin banner) -----
sudo nc -l 127.0.0.1 25 >/dev/null 2>&1 &
PID_NC_25=$!
echo "    - netcat en 25 (pid $PID_NC_25)"

# ----- 4) netcat en 80 (HTTP falso, sin banner) -----
sudo nc -l 127.0.0.1 80 >/dev/null 2>&1 &
PID_NC_80=$!
echo "    - netcat en 80 (pid $PID_NC_80)"

# ----- 5) netcat en 21 (FTP de la tabla) enviando banner incorrecto “BADFTP” -----
#      Esto obligará a que msg “FTP (comportamiento NO coincide)”
(
  echo "BADFTP"
  sleep 60
) | nc -l 127.0.0.1 21 >/dev/null 2>&1 &
PID_NC_21_BAD=$!
echo "    - netcat en 21 (FTP, banner incorrecto) (pid $PID_NC_21_BAD)"

# ----- 6) netcat en 22 (SSH de la tabla) enviando banner “NOTSSH” -----
#      Para que “SSH (comportamiento NO coincide)”
(
  echo "NOTSSH"
  sleep 60
) | nc -l 127.0.0.1 22 >/dev/null 2>&1 &
PID_NC_22_BAD=$!
echo "    - netcat en 22 (SSH, banner incorrecto) (pid $PID_NC_22_BAD)"

# ----- 7) netcat en 4444 (puerto no en tabla) -> servidor HTTP real -----
python3 -m http.server 4444 --bind 127.0.0.1 >/dev/null 2>&1 &
PID_HTTP_4444=$!
echo "    - HTTP real en 4444 (puerto no estándar) (pid $PID_HTTP_4444)"

# ----- 8) netcat en 2222 (puerto no en tabla) -> servidor SSH simulado ----- 
#      Enviamos un banner típico de SSH (“SSH-2.0-…”) para que la heurística lo detecte
(
  echo "SSH-2.0-FakeSSH_0.1"
  sleep 60
) | nc -l 127.0.0.1 2222 >/dev/null 2>&1 &
PID_NC_2222_SSH=$!
echo "    - SSH simulado en 2222 (puerto no estándar) (pid $PID_NC_2222_SSH)"

# ----- 9) netcat en 2121 (puerto no en tabla) -> servidor FTP simulado -----
#      Enviamos un banner “220 FakeFTP” para que la heurística lo detecte
(
  echo "220 FakeFTP"
  sleep 60
) | nc -l 127.0.0.1 2121 >/dev/null 2>&1 &
PID_NC_2121_FTP=$!
echo "    - FTP simulado en 2121 (puerto no estándar) (pid $PID_NC_2121_FTP)"

# ----- 10) (POP3 110: no levantamos nada, queda cerrado) -----
echo "    - (POP3 110 permanece cerrado)"

# ----- 11) (HTTPS 8443: si tienes TLS manual, no lo levantamos) -----
echo "    - (HTTPS 8443: depende de que manualmente inicies un servidor TLS)"

# Esperar un par de segundos para que todos los listeners queden listos
sleep 2

echo
echo "🧪 Escaneando todos los puertos de golpe:"
./bin/escaner
STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "❌ El escáner terminó con error (código $STATUS)"
    # Matamos procesos de prueba antes de salir
    kill $PID_HTTP_8080       2>/dev/null
    kill $PID_NC_31337        2>/dev/null
    sudo kill $PID_NC_25      2>/dev/null
    sudo kill $PID_NC_80      2>/dev/null
    kill $PID_NC_21_BAD       2>/dev/null
    kill $PID_NC_22_BAD       2>/dev/null
    kill $PID_HTTP_4444       2>/dev/null
    kill $PID_NC_2222_SSH     2>/dev/null
    kill $PID_NC_2121_FTP     2>/dev/null
    exit $STATUS
fi

# Esperar un segundo antes de matar los procesos
sleep 1

# Una vez escaneado, matamos todos los procesos que levantamos:
kill $PID_HTTP_8080       2>/dev/null
kill $PID_NC_31337        2>/dev/null
sudo kill $PID_NC_25      2>/dev/null
sudo kill $PID_NC_80      2>/dev/null
kill $PID_NC_21_BAD       2>/dev/null
kill $PID_NC_22_BAD       2>/dev/null
kill $PID_HTTP_4444       2>/dev/null
kill $PID_NC_2222_SSH     2>/dev/null
kill $PID_NC_2121_FTP     2>/dev/null

echo "✅ Escáner ejecutado correctamente"
exit 0