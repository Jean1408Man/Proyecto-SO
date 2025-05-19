/* 1. Lee /proc/stat y guarda jiffies totales
   2. Recorre /proc/[pid]/stat para jiffies por proceso
   3. Calcula Δ y CPU %, compara con umbral
   4. Lee /proc/[pid]/status para VmRSS y compara
   5. Encola pm_alert_t si hay pico
*/

#include "process_monitor.h"
#include <dirent.h> 
