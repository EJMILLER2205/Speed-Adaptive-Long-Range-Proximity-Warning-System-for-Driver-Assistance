#include "hal.h"
#include "tf03.h"
#include "gps.h"
#include "filter.h"
#include "warn.h"
#include <stdio.h>

#define LIDAR_DT 0.01f
#define LIDAR_TIMEOUT_MS 200
#define GPS_TIMEOUT_MS 1000
#define LOG_PERIOD_MS 100
#define LIDAR_MAX_CM 10000

int main(void)
{
	// Creates structures
    tf03_parser_t lidar;
	gps_parser_t gps;
	filter_t filt;
	warn_t warner;
	
	// Variables that will be used
	float gps_speed = 0;
	uint32_t last_lidar_ms = 0;
	uint32_t last_gps_ms = 0;
	uint32_t last_log_ms = 0;
	
	// Setup
	hal_init();
	tf03_init(&lidar);
	gps_init(&gps);
	filter_init(&filt, 5.0f, 0.10f, 2.0f);
	warn_init(&warner);
	led_set(LED_POWER, true);
	log_print("time_ms,range_cm,rate_cm,gps_cms,state,beep\n");
	
    while (1) 
    {
		// Get time
		uint32_t now = millis();
		
		// Read GPS
		while(gps_available()) {
			char c = gps_read();
			float speed;
			if(gps_feed(&gps, c, &speed)) {
				gps_speed = speed;
				last_gps_ms = now;
			}
		}
		
		// Read LiDAR
		while(lidar_available()) {
			uint8_t byte = lidar_read();
			uint16_t distance;
			if(tf03_feed(&lidar, byte, &distance)) {
				if (distance > 0 && distance <= LIDAR_MAX_CM) {
					filter_update(&filt, distance / 100.0f, LIDAR_DT);
					last_lidar_ms = now;
				}
			}
		}
		
		// Check whats still working
		bool lidar_ok = (now - last_lidar_ms < LIDAR_TIMEOUT_MS) && filt.ready;
		bool gps_ok = (now - last_gps_ms < GPS_TIMEOUT_MS);
		
		// If LiDAR timed out, reset the filter
		if (!lidar_ok) {
			filt.ready = false;
		}
		
		// If GPS is lost, use 0 for speed
		float own_speed = gps_ok ? gps_speed : 0.0f;
		
		// Decides if it should warn
		bool beep = warn_update(&warner, lidar_ok, filter_range(&filt), filter_rate(&filt), own_speed);
		
		// Act on warnings
		if (beep) {
			tone_on();
		}
		else {
			tone_off();
		}
		
		// Sets LEDs
		led_set(LED_LIDAR, lidar_ok);
		led_set(LED_GPS, gps_ok);
		led_set(LED_WARNING, beep);
		
		// Log every 100ms
		if(now - last_log_ms >= LOG_PERIOD_MS) {
			last_log_ms = now;
			char line[64];
			snprintf(line, sizeof line, "%lu,%d,%d,%d,%d,%d\n", 
				(unsigned long)now, 
				(int)(filter_range(&filt) * 100.0f), 
				(int)(filter_rate(&filt) * 100.0f), 
				(int)(own_speed * 100.0f), 
				(int)(warner.state), 
				(int)(beep));
			log_print(line);
		}
    }
}

