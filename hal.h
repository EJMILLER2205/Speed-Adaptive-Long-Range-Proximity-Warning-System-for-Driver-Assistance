#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	LED_POWER,
	LED_LIDAR,
	LED_GPS,
	LED_WARNING
	} led_t;

void hal_init(void);
uint32_t millis(void);
bool lidar_available(void);
uint8_t lidar_read(void);
bool gps_available(void);
char gps_read(void);
void gps_write(const uint8_t *data, uint8_t len);
void log_print(const char *text);
void tone_on(void);
void tone_off(void);
void led_set(led_t which, bool on);

#endif
