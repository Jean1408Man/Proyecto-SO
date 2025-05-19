#include "process_monitor.h"

int pm_init(const pm_config_t *cfg) {
    (void)cfg;
    return 0;
}

int pm_sample(void) {
    return 0; 
}

int pm_get_alert(pm_alert_t *out) {
    (void)out;
    return 0;
}

void pm_shutdown(void) {
}
