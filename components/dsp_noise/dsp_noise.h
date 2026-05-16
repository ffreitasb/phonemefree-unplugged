#pragma once

#include <stddef.h>
#include <stdint.h>

#define DSP_NOISE_DEFAULT_SEED 0xACE1u

void dsp_noise_reset(uint32_t seed);
void dsp_noise_mix(int16_t *samples, size_t num_samples, uint8_t level_pct);
