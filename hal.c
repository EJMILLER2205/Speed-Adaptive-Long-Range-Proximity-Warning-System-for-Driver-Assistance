#include "hal.h"

void hal_init(void) { }
uint32_t millis(void) { return 0; }
bool lidar_available(void) { return false; }
uint8_t lidar_read(void) { return 0; }
bool gps_available(void) { return false; }
char gps_read(void) { return 0; }
void gps_write(const uint8_t *data, uint8_t len) { (void)data; (void)len; }
void log_print(const char *text) { (void)text; }
void tone_on(void) { }
void tone_off(void) { }
void led_set(led_t which, bool on) { (void)which; (void)on; }
