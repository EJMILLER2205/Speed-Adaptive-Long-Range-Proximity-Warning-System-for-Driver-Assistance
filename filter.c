#include "filter.h"

#include <math.h>

// Initialize filter
void filter_init(filter_t *f, float sigma_a, float sigma_meas, float gate_m) {
	f->q = sigma_a * sigma_a;
	f->rm = sigma_meas * sigma_meas;
	f->gate_m = gate_m;
	f->misses = 0;
	f->ready = false;
}

// Calculates predictions
void filter_predict(filter_t *f, float dt) {
	f->r = f->r + f->v * dt;
	f->p11 = f->p11 + dt * (2 * f->p12 + dt * f->p22) + f->q * dt * dt * dt * dt / 4;
	f->p12 = f->p12 + dt * f->p22 + f->q * dt * dt * dt / 2;
	f->p22 = f->p22 + f->q * dt * dt;
}

// Update with new value
bool filter_update(filter_t *f, float range_m, float dt) {
	if (f->ready == false) {
		f->r = range_m;
		f->v = 0;
		f->p11 = f->rm;
		f->p12 = 0;
		f->p22 = 100;
		f->ready = true;
		f->misses = 0;
		return true;
	}
	
	filter_predict(f, dt);
	float compare = range_m - f->r;
	if(fabsf(compare) > f->gate_m) {
		f->misses++;
		if(f->misses > 30) {
			f->ready = false;
		}
		return false;
	}
	f->misses = 0;
	
	float total_uncertainty = f->p11 + f->rm;
	float k1 = f->p11 / total_uncertainty;
	float k2 = f->p12 / total_uncertainty;
	
	f->r = f->r + k1 * compare;
	f->v = f->v + k2 * compare;
	
	f->p22 = f->p22 - k2 * f->p12;
	f->p12 = f->p12 - k1 * f->p12;
	f->p11 = f->p11 - k1 * f->p11;
	
	return true;
}

// Range getter
float filter_range(const filter_t *f) {
	return f->r;
}

// Rate getter
float filter_rate(const filter_t *f) {
	return f->v;
}
