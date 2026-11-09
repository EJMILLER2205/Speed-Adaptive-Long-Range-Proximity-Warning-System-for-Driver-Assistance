#include "gps.h"
#include <stdlib.h>

#define KMH_TO_MPS (1.0f / 3.6f)
#define KNOTS_TO_MPS 0.514444f

//---------------HELPERS--------------------//

// Turns hex character into its value
static uint8_t hex_value(char c) {
	if(c >= '0' && c <= '9') return (uint8_t)(c - '0');
	if(c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
	if(c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
	return 0xFF;
}

// Checks the *XX at the end of the line
static bool checksum_ok(const char *line) {
	uint8_t sum = 0;
	const char *s = line + 1;
	while (*s != '\0' && *s != '*') {
		sum ^= (uint8_t)*s;
		s++;
	}
	if (*s != '*') return false;
	
	uint8_t hi = hex_value(s[1]);
	uint8_t lo = (hi == 0xFF) ? 0xFF : hex_value(s[2]);
	if (hi == 0xFF || lo == 0xFF) return false;
	
	return sum == (uint8_t)((hi << 4) | lo);
}

// Returns a pointer to start of field n
static const char *get_field(const char *line, uint8_t n) {
	const char *s = line + 1;
	while(n > 0) {
		while(*s != ',' && *s != '*' && *s != '\0') s++;
		if (*s != ',') return NULL;
		s++;
		n--;
	}
	return s;
}

// True if field has nothing in it
static bool field_empty(const char *f) {
	return f == NULL || *f == ',' || *f == '*' || *f == '\0';
}

// True if sentence type matches
static bool is_type(const char *line, const char *type) {
	return line[3] == type[0] && line[4] == type[1] && line[5] == type[2] && line[6] == ',';
}

// Reads a number from a field. Returns false if theres no number there
static bool read_number(const char *f, float *out) {
	if(field_empty(f)) return false;
	char *end;
	double v = strtod(f, &end);
	if (end == f) return false;
	*out = (float)v;
	return true;
}

// VTG, field 7 = speed in km/h, field 9 = mode (A or D = valid, N = no)
static bool parse_vtg(const char *line, float *speed_mps) {
	const char *mode = get_field(line, 9);
	if(field_empty(mode) || (*mode != 'A' && *mode != 'D')) return false;
	float kmh;
	if (!read_number(get_field(line, 7), &kmh)) return false;
	*speed_mps = kmh * KMH_TO_MPS;
	return true;
}

// RMC, field 2 = status (A = valid), field 7 = speed in knots
static bool parse_rmc(const char *line, float *speed_mps) {
	const char *status = get_field(line, 2);
	if (field_empty(status) || *status != 'A') return false;
	float knots;
	if(!read_number(get_field(line, 7), &knots)) return false;
	*speed_mps = knots * KNOTS_TO_MPS;
	return true;
}

//-------------------PUBLIC FUNCTIONS------------//

void gps_init(gps_parser_t *p) {
	p->len = 0;
	p->in_sentence = false;
}

bool gps_feed(gps_parser_t *p, char c, float *speed_mps) {
	// First character must be $
	if (c == '$') {
		p->in_sentence = true;
		p->len = 0;
		p->buf[p->len++] = c;
		return false;
	}
	
	// If $ hasn't been seen
	if(!p->in_sentence) {
		return false;
	}
	
	// If end of line
	if (c == '\r' || c == '\n') {
		p->buf[p->len] = '\0';
		p->in_sentence = false;
		if(p->len < 7) {
			return false;
		}
		if(!checksum_ok(p->buf)) {
			return false;
		}
		if (is_type(p->buf, "VTG")) {
			return parse_vtg(p->buf, speed_mps);
		}
		if (is_type(p->buf, "RMC")) {
			return parse_rmc(p->buf, speed_mps);
		}
		return false;
	}
	
	// Line too long, throw away
	if (p->len >= MAX_GPS_LEN - 1) {
		p->in_sentence = false;
		return false;
	}
	
	// Otherwise, store character
	p->buf[p->len++] = c;
	return false;
}
