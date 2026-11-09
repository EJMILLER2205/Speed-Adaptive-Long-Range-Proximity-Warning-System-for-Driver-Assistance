#ifndef GPS_H
#define GPS_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_GPS_LEN 83

typedef struct {
	char buf[MAX_GPS_LEN];
	uint8_t len; // how many characters are in buffer
	bool in_sentence; // true after seeing $
	} gps_parser_t;
	
void gps_init(gps_parser_t *p);
bool gps_feed(gps_parser_t *p, char c, float *speed_mps);

#endif
