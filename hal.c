#define F_CPU 24000000UL
#include "hal.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define BAUD_REG(b) ((uint16_t)((4UL * F_CPU + (b) / 2) / (b)))
#define LIDAR_BAUD 115200UL
#define GPS_BAUD 115200UL
#define LOG_BAUD 115200UL

static volatile uint32_t ms_count;

// Timer interrupt
ISR(TCB0_INT_vect) {
	// Adds ms
	ms_count++;
	// Clears interrupt
	TCB0.INTFLAGS = TCB_CAPT_bm;
}

// Initialize
void hal_init(void) {
	// Sets oscillation to 24Mhz
	_PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);
	// 23999 ticks between interrupts
	TCB0.CCMP = F_CPU / 1000 - 1;
	// Set count mode to periodic interrupt
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;
	// Turns on captive interrupt
	TCB0.INTCTRL = TCB_CAPT_bm;
	// Set clock to divide by one and enable timer
	TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;
	
	// Set up LED pins
	// Onboard LED
	PORTB.DIRSET = PIN3_bm;
	PORTB.OUTSET = PIN3_bm;
	//External LEDS
	PORTD.DIRSET = PIN1_bm | PIN2_bm | PIN3_bm;
	PORTD.OUTCLR = PIN1_bm | PIN2_bm | PIN3_bm;
	
	// Interrupt
	sei();
}

// Allows copying of ms_count without interrupt changing it
uint32_t millis(void) {
	uint32_t m;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		m = ms_count;
	}
	return m;
}

bool lidar_available(void) { return false; }
uint8_t lidar_read(void) { return 0; }
bool gps_available(void) { return false; }
char gps_read(void) { return 0; }
void gps_write(const uint8_t *data, uint8_t len) { (void)data; (void)len; }
void log_print(const char *text) { (void)text; }
void tone_on(void) { }
void tone_off(void) { }
	
// Turns one of the LEDS on or off
void led_set(led_t which, bool on) {
	switch(which) {
		case LED_POWER:
			if (on) {
				PORTB.OUTCLR = PIN3_bm;
			}
			else {
				PORTB.OUTSET = PIN3_bm;
			}
			break;
		case LED_LIDAR:
			if (on) {
				PORTD.OUTSET = PIN1_bm;
			}
			else {
				PORTD.OUTCLR = PIN1_bm;
			}
			break;
		case LED_GPS:
			if (on) {
				PORTD.OUTSET = PIN2_bm;
			}
			else {
				PORTD.OUTCLR = PIN2_bm;
			}
			break;
		case LED_WARNING:
			if (on) {
				PORTD.OUTSET = PIN3_bm;
			}
			else {
				PORTD.OUTCLR = PIN3_bm;
			}
			break;
	}
}
