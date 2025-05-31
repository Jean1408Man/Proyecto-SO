echo "🧪 Iniciando todos los servicios/nc listeners de prueba..."

# ----- 1) Servidor HTTP en 8080 -----
python3 -m http.server 8080 --bind 127.0.0.1 >/dev/null 2>&1 &
PID_HTTP=$!
echo "    - HTTP real en 8080 (pid $PID_HTTP)"

# ----- 2) netcat en 31337 -----
nc -l 127.0.0.1 31337 >/dev/null 2>&1 &
PID_NC_31337=$!
echo "    - netcat en 31337 (pid $PID_NC_31337)"

# ----- 3) netcat en 25 (SMTP simulado) -----
sudo nc -l 127.0.0.1 25 >/dev/null 2>&1 &
PID_NC_25=$!
echo "    - netcat en 25 (pid $PID_NC_25)"

# ----- 4) netcat en 80 (HTTP falso) -----
sudo nc -l 127.0.0.1 80 >/dev/null 2>&1 &
PID_NC_80=$!
echo "    - netcat en 80 (pid $PID_NC_80)"

# ----- (POP3 110: no levantar nada, queda cerrado) -----
echo "    - (POP3 110 permanece cerrado)"

# ----- (SSH en 22: asumimos que si está activo, ya está levantado; si no, no hará nada) -----
echo "    - SSH 22: dependerá de que sshd esté activo o no en 127.0.0.1"

# ----- (HTTPS 8443: si tienes un TLS activo, debe estarlo manualmente; sino no lo abrimos) -----
echo "    - (HTTPS 8443: depende de que manualmente hayas iniciado un servidor TLS allí)"

# Esperar un par de segundos para que todos los listeners queden listos
sleep 2

echo
echo "🧪 Escaneando todos los puertos de golpe:"
./bin/escaner

sleep 1 

# Una vez escaneado, matamos todos los procesos que levantamos:
kill $PID_HTTP    2>/dev/null
kill $PID_NC_31337 2>/dev/null
sudo kill $PID_NC_25  2>/dev/null
sudo kill $PID_NC_80  2>/dev/null

# echo
# echo "🧪 Resultados esperados (todos juntos):"
# echo
# echo "— Puerto 8080 (HTTP real) —"
# echo "PUERTO ABIERTO: 8080 → Servicio: HTTP ✅ esperado"
# echo
# echo "— Puerto 31337 (no en tabla) —"
# echo "⚠️ PUERTO ABIERTO: 31337 → DESCONOCIDO (NO en tabla)"
# echo "    ↪ PID: <pid de netcat en 31337>, Usuario: <tu_usuario>, Programa: /usr/bin/nc"
# echo
# echo "— Puerto 25 (netcat, simulando SMTP falso) —"
# echo "⚠️ PUERTO ABIERTO: 25 → Servicio esperado: SMTP, pero comportamiento NO coincide"
# echo "    ↪ No se recibió banner del servicio."
# echo "    ↪ PID: <pid de netcat en 25>, Usuario: <tu_usuario>, Programa: /usr/bin/nc"
# echo
# echo "— Puerto 80 (netcat, simulando HTTP falso) —"
# echo "⚠️ PUERTO ABIERTO: 80 → Servicio esperado: HTTP, pero comportamiento NO coincide"
# echo "    ↪ No se recibió banner del servicio."
# echo "    ↪ PID: <pid de netcat en 80>, Usuario: <tu_usuario>, Programa: /usr/bin/nc"
# echo
# echo "— Puerto 110 (POP3) —"
# echo "(no debe imprimirse ninguna línea porque está cerrado)"
# echo
# echo "— Puerto 22 (SSH) —"
# echo "Si SSH está activo en 127.0.0.1:22, debería verse:"
# echo "PUERTO ABIERTO: 22 → Servicio: SSH ✅ esperado"
# echo "Si no está, simplemente no sale nada para 22."
# echo
# echo "— Puerto 8443 (HTTPS) —"
# echo "Si tienes un servidor TLS corriendo en 8443, debería verse:"
# echo "PUERTO ABIERTO: 8443 → Servicio: HTTPS ✅ esperado"
# echo "O bien, si algo abierto pero no responde correctamente, algo como:"
# echo "⚠️ PUERTO ABIERTO: 8443 → Servicio esperado: HTTPS, pero comportamiento NO coincide"
# echo "    ↪ Banner recibido: <datos binarios o vacío>"
# echo "    ↪ PID: <pid del proceso TLS>, Usuario: <tu_usuario>, Programa: <ruta del ejecutable TLS>"
# echo
# echo "✅ Fin de los resultados esperados."
