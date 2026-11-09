#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	// Estimate
	float r; // Gap to target in meters
	float v; // Rate the gap is changing in m/s
	
	// Uncertainty
	float p11; // How unsure gap estimate is
	float p22; // How unsure rate estimate is
	float p12; // How the two uncertainties are linked
	
	// Settings
	float q; // How much to expect rate to change (acceleration noise squared)
	float rm; // How noisy the LiDAR is (measurement noise squared)
	float gate_m; // Biggest jump in meters thats believable
	
	// Bookkeeping
	uint8_t misses; // How many readings in a row were rejected
	bool ready; // False until first reading arrives
	} filter_t;

void filter_init(filter_t *f, float sigma_a, float sigma_meas, float gate_m);
bool filter_update(filter_t *f, float range_m, float dt);
void filter_predict(filter_t *f, float dt);
float filter_range(const filter_t *f);
float filter_rate(const filter_t *f);

#endif
