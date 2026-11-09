#ifndef TF03_H
#define TF03_H

#include <stdint.h>
#include <stdbool.h>

#define TF03_FRAME_LEN 9
#define TF03_HEADER 0x59

typedef struct {
	uint8_t buf[TF03_FRAME_LEN]; // Bytes currently collected
	uint8_t idx; // Amount of bytes currently in buffer
	} tf03_parser_t;
	
void tf03_init(tf03_parser_t *p); // Resets parser to wait for new frame
bool tf03_feed(tf03_parser_t *p, uint8_t byte, uint16_t *dist_cm); // Gives the parser one byte, returns true when byte finishes a valid reading

#endif
