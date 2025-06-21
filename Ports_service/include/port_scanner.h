#ifndef PORT_SCANNER_H
#define PORT_SCANNER_H

#include "models.h"

/**
 * Escanea puertos TCP en localhost, desde start_port hasta end_port (inclusive).
 * Devuelve un ScanResult con un arreglo de ScanOutput (solo puertos abiertos).
 * El llamador es responsable de liberar cada banner y cada dangerous_word,
 * y finalmente el arreglo data.
 */
ScanResult scan_ports(int start_port, int end_port);

#endif // PORT_SCANNER_H
