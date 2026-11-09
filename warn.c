#include "warn.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Initialize warning
void warn_init(warn_t *w) {
	w->state = TARGET_NONE;
	w->seen = 0;
	w->warning = false;	
	w->t_react = 1.5f;
	w->decel = 5.5f;
	w->static_thresh = 2.0f;
	w->min_speed = 5.0f;
}

// Formula to figure out if its safe distance or not
float warn_safe_distance(const warn_t *w, float v_own, float v_lead) {
	// If object in front is going backwards count as noise
	if (v_lead < 0) {
		v_lead = 0;
	}
	
	// If car in front is moving faster
	float extra = (v_own * v_own - v_lead * v_lead) / (2 * w->decel);
	if(extra < 0){
		extra = 0;
	}
	
	float d = v_own * w->t_react + extra;
	return d;
}

// Decides if it should warn or not
bool warn_update(warn_t *w, bool target_ok, float range_m, float rate, float v_own) {
	// If target was lost
	if (target_ok == false) {
		w->state = TARGET_NONE;
		w->seen = 0;
		w->warning = false;
		return false;
	}
	
	// Ground speed
	float v_lead = v_own + rate;
	
	// Confirm target is real
	if(w->seen < 20) {
		w->seen++;
		return false;
	}
	
	// Classify with memory
	if (w->state == TARGET_NONE) {
		if (fabsf(v_lead) < w->static_thresh) {
			w->state = TARGET_STATIC;
		}
		else {
			w->state = TARGET_VEHICLE;
		}
	}
	if (w->state == TARGET_STATIC && v_lead > w->static_thresh) {
		w->state = TARGET_VEHICLE;
	}
		// If already vehicle it will stay vehicle
	
	// Too slow to matter
	if (v_own < w->min_speed) {
		w->warning = false;
		return false;
	}
	
	// Only cars trigger warnings
	if (w->state != TARGET_VEHICLE) {
		w->warning = false;
		return false;
	}
	
	// Decides if warning should be issued
	float d_safe = warn_safe_distance(w, v_own, v_lead);
	if(range_m < d_safe && w->warning == false) {
		w->warning = true;
	}
	if(range_m > d_safe * 1.1f && w->warning == true) {
		w->warning = false;
	}
	
	// returns warning
	return w->warning;
}
