
#include "tf03.h"

void tf03_init(tf03_parser_t *p) {
	p->idx = 0;
}

bool tf03_feed(tf03_parser_t *p, uint8_t byte, uint16_t *dist_cm) {
	// First 2 bytes must be 0x59
	if (p->idx < 2 && byte != TF03_HEADER) {
		p->idx = 0;
		return false;
	}
	
	// Write the byte down
	p->buf[p->idx] = byte;
	p->idx++;
	
	// Waits for 9 bytes
	if(p->idx < 9) {
		return false;
	}
	
	// Reset for next frame
	p->idx = 0;
	
	// Checksum, add bytes 0-7
	uint8_t sum = 0;
	for(uint8_t i = 0; i < TF03_FRAME_LEN - 1; i++) {
		sum += p->buf[i];
	}
	
	// If its corrupted, return false as garbage
	if (sum != p->buf[TF03_FRAME_LEN - 1]) {
		return false;
	}
	
	// Finds the distance
	*dist_cm = (uint16_t)(p->buf[2] | ((uint16_t)p->buf[3] << 8));
	return true;
}
