#!/bin/bash

mkdir -p tests
echo "🧪 Ejecutando pruebas de escaneo de puertos..."

# ===== CASO 1: HTTP en puerto 8080 (válido, puerto en tabla) =====
echo "[*] Caso 1: HTTP en puerto 8080"
python3 -m http.server 8080 >/dev/null 2>&1 &
PID1=$!
sleep 2
./escaner > tests/out_8080.out
kill $PID1
echo "[✓] Resultado esperado:"
echo "PUERTO ABIERTO: 8080 → Servicio: HTTP ✅ esperado"

# ===== CASO 2: Puerto 31337 no común =====
# ⚠️ solo aparecerá si está en puertos_extra[]
echo "[*] Caso 2: netcat en puerto 31337 (no en tabla)"
nc -l 31337 >/dev/null 2>&1 &
PID2=$!
sleep 2
./escaner > tests/out_31337.out
kill $PID2
echo "[✓] Resultado esperado:"
echo "⚠️ PUERTO ABIERTO: 31337 → DESCONOCIDO (NO en tabla)"
echo "    ↪ PID: ..., Usuario: ..., Programa: ..."

# ===== CASO 3: Puerto 25 abierto pero sin comportamiento SMTP =====
echo "[*] Caso 3: netcat en puerto 25 (esperado SMTP, pero falso)"
sudo nc -l 25 >/dev/null 2>&1 &
PID3=$!
sleep 2
./escaner > tests/out_25_nc.out
sudo kill $PID3
echo "[✓] Resultado esperado:"
echo "⚠️ PUERTO ABIERTO: 25 → Servicio esperado: SMTP, pero comportamiento NO coincide"
echo "    ↪ Banner recibido: "
echo "    ↪ PID: ..., Usuario: ..., Programa: ..."

# ===== CASO 4: POP3 cerrado =====
echo "[*] Caso 4: POP3 cerrado (puerto 110)"
./escaner > tests/out_110.out
echo "[✓] Resultado esperado:"
echo "Puerto 110/tcp cerrado ✅"

# ===== CASO 5: HTTP simulado en puerto 80 con netcat =====
echo "[*] Caso 5: netcat en puerto 80 (esperado HTTP, comportamiento falso)"
sudo nc -l 80 >/dev/null 2>&1 &
PID5=$!
sleep 2
./escaner > tests/out_80_nc.out
sudo kill $PID5
echo "[✓] Resultado esperado:"
echo "⚠️ PUERTO ABIERTO: 80 → Servicio esperado: HTTP, pero comportamiento NO coincide"
echo "    ↪ Banner recibido: "
echo "    ↪ PID: ..., Usuario: ..., Programa: ..."

# ===== CASO 6: SSH en puerto 22 (válido si activo) =====
echo "[*] Caso 6: SSH en puerto 22 (si está activo)"
./escaner > tests/out_22.out
echo "[✓] Resultado esperado:"
echo "PUERTO ABIERTO: 22 → Servicio: SSH ✅ esperado"

# ===== CASO 7: HTTPS en 8443 si está activado manualmente =====
# (debes tener un servidor TLS en 8443 si quieres validar este caso)
echo "[*] Caso 7: HTTPS en 8443 (si tienes TLS ahí)"
./escaner > tests/out_8443.out
echo "[✓] Resultado esperado (si coincide con comportamiento):"
echo "PUERTO ABIERTO: 8443 → Servicio: HTTPS ✅ esperado"
echo "⚠️ o bien: comportamiento NO coincide + banner + PID info"

# ===== FINAL =====
echo "✅ Todas las pruebas finalizadas. Resultados guardados en la carpeta tests/"
