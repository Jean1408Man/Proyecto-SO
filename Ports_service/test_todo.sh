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
STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "❌ El escáner terminó con error (código $STATUS)"
    # Aún así matamos los procesos y salimos con el mismo código
    kill $PID_HTTP 2>/dev/null
    kill $PID_NC_31337 2>/dev/null
    sudo kill $PID_NC_25 2>/dev/null
    sudo kill $PID_NC_80 2>/dev/null
    exit $STATUS
fi


sleep 1 

# Una vez escaneado, matamos todos los procesos que levantamos:
kill $PID_HTTP    2>/dev/null
kill $PID_NC_31337 2>/dev/null
sudo kill $PID_NC_25  2>/dev/null
sudo kill $PID_NC_80  2>/dev/null

echo "✅ Escáner ejecutado correctamente"
exit 0