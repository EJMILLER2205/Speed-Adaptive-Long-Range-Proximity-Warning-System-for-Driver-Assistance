#ifndef WARN_H
#define WARN_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	TARGET_NONE,
	TARGET_STATIC,
	TARGET_VEHICLE
	} target_state_t;
	
typedef struct {
	float t_react; // Reaction time 1.5s
	float decel; // Braking deceleration (5.5m/s^2)
	float static_thresh; // If below this ground speed, its considered static (2m/s)
	float min_speed; // Below this own speed, never warn (5m/s)
	target_state_t state; // What target is
	uint8_t seen; // How many readings in a row target has been seen
	bool warning; // Whether the warning is active or not
} warn_t;

void warn_init(warn_t *w);
float warn_safe_distance(const warn_t *w, float v_own, float v_lead);
bool warn_update(warn_t *w, bool target_ok, float range_m, float rate, float v_own);

#endif
