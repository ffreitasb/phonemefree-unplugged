#pragma once

#include <stddef.h>
#include <stdint.h>

void dsp_noise_mix(int16_t *samples, size_t num_samples, uint8_t level_pct);
