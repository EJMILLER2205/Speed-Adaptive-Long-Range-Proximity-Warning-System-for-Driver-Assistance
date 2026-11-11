#define F_CPU 24000000UL
#include "hal.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define BAUD_REG(b) ((uint16_t)((4UL * F_CPU + (b) / 2) / (b)))
#define LIDAR_BAUD 115200UL
#define GPS_BAUD 115200UL
#define LOG_BAUD 115200UL
#define RB_SIZE 64
#define RB_MASK (RB_SIZE - 1)

typedef struct {
	volatile uint8_t data[RB_SIZE];
	volatile uint8_t head;
	volatile uint8_t tail;
	} ringbuf_t;

static volatile uint32_t ms_count;
static ringbuf_t lidar_rb;
static ringbuf_t gps_rb;
static volatile bool tone_high;
static bool tone_running;

//------------------------HELPERS--------------------//
// Adds a byte to a ring buffer, drops if full
static void rb_put(ringbuf_t *rb, uint8_t b) {
	uint8_t next = (rb->head + 1) & RB_MASK;
	if (next != rb->tail) {
		rb->data[rb->head] = b;
		rb->head = next;
	}
}

// True of the ring buffer has nothing in it
static bool rb_empty(const ringbuf_t *rb) {
	return rb->head == rb->tail;
}

// Takes the next byte out of ring buffer
static uint8_t rb_get(ringbuf_t *rb) {
	uint8_t b = rb->data[rb->tail];
	rb->tail = (rb->tail + 1) & RB_MASK;
	return b;
}

// Sends one byte out of a USART, waiting until it has room
static void uart_send(USART_t *u, uint8_t b) {
	while (!(u->STATUS & USART_DREIF_bm)) {
		
	}
	u->TXDATAL = b;
}

//----------------------------INTERRUPTS---------------//

// Timer interrupt
ISR(TCB0_INT_vect) {
	// Adds ms
	ms_count++;
	// Clears interrupt
	TCB0.INTFLAGS = TCB_CAPT_bm;
}

// LiDAR byte received
ISR(USART1_RXC_vect) {
	rb_put(&lidar_rb, USART1.RXDATAL);
}

// GPS byte received
ISR(USART4_RXC_vect) {
	rb_put(&gps_rb, USART4.RXDATAL);
}

// Tone timer
ISR(TCB1_INT_vect) {
	tone_high = !tone_high;
	DAC0.DATA = (tone_high ? 768 : 256) << 6;
	TCB1.INTFLAGS = TCB_CAPT_bm;
}

//------------------------MAIN FUNCTIONS-------------------//

// Initialize
void hal_init(void) {
	// Sets oscillation to 24Mhz
	_PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);
	// 23999 ticks between interrupts
	TCB0.CCMP = F_CPU / 1000 - 1;
	// Set count mode to periodic interrupt
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;
	// Turns on capture interrupt
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
	
	// LiDAR UART (USART1)
	PORTC.DIRSET = PIN0_bm;
	USART1.BAUD = BAUD_REG(LIDAR_BAUD);
	USART1.CTRLA = USART_RXCIE_bm;
	USART1.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

	// GPS UART (USART4)
	PORTE.DIRSET = PIN0_bm;
	USART4.BAUD = BAUD_REG(GPS_BAUD);
	USART4.CTRLA = USART_RXCIE_bm;
	USART4.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

	// Log UART (USART3)
	PORTB.DIRSET = PIN0_bm;
	USART3.BAUD = BAUD_REG(LOG_BAUD);
	USART3.CTRLB = USART_TXEN_bm;

	// DAC on PD6
	PORTD.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	VREF.DAC0REF = VREF_REFSEL_VDD_gc;
	DAC0.DATA = 512 << 6;
	DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;

	// Tone timer
	TCB1.CCMP = F_CPU / 4000 - 1;
	TCB1.CTRLB = TCB_CNTMODE_INT_gc;
	TCB1.INTCTRL = TCB_CAPT_bm;
	
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

bool lidar_available(void) {
	return !rb_empty(&lidar_rb);
}

uint8_t lidar_read(void) {
	return rb_get(&lidar_rb);
}

bool gps_available(void) {
	return !rb_empty(&gps_rb);
}

char gps_read(void) {
	return (char)rb_get(&gps_rb);
}

void gps_write(const uint8_t *data, uint8_t len) {
	for(uint8_t i = 0; i < len; i++) {
		uart_send(&USART4, data[i]);
	}
}

void log_print(const char *text) {
	while(*text != '\0') {
		uart_send(&USART3, (uint8_t)*text);
		text++;
	}
}

void tone_on(void) {
	if(tone_running) {
		return;
	}
	tone_running = true;
	TCB1.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;
}

void tone_off(void) {
	if(!tone_running) {
		return;
	}
	tone_running = false;
	TCB1.CTRLA = 0;
	DAC0.DATA = 512 << 6;
}

	
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
