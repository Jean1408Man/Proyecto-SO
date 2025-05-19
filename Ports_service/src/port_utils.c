#include "port_utils.h"
#include <glib.h>

GHashTable* inicializar_tabla_puertos(void) {
    GHashTable* tabla = g_hash_table_new(g_int_hash, g_int_equal);

    static int puertos[] = {
        20,21,22,23,25,53,67,68,69,70,79,80,
        110,111,123,135,137,138,139,143,161,162,
        179,194,389,443,445,465,514,515,587,631,
        636,993,995,1024
    };
    const char* servicios[] = {
        "FTP-DATA","FTP","SSH","Telnet","SMTP","DNS","DHCP Server","DHCP Client","TFTP",
        "Gopher","Finger","HTTP","POP3","RPCbind","NTP","MS RPC","NetBIOS NS","NetBIOS DGM",
        "NetBIOS SSN","IMAP","SNMP","SNMP Trap","BGP","IRC","LDAP","HTTPS","SMB",
        "SMTPS","Syslog","LPD/LPR","SMTP (STARTTLS)","IPP","LDAPS","IMAPS","POP3S"
    };

    int n = sizeof(puertos)/sizeof(puertos[0]);
    for (int i = 0; i < n; ++i) {
        int *clave = g_new(int, 1);
        *clave = puertos[i];
        g_hash_table_insert(tabla, clave, (gpointer)servicios[i]);
    }

    return tabla;
}

const char* buscar_servicio(GHashTable *tabla, int puerto) {
    gpointer v = g_hash_table_lookup(tabla, &puerto);
    return v ? (const char*)v : NULL;
}
