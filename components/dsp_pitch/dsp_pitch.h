#pragma once

#include <stddef.h>
#include <stdint.h>

void dsp_pitch_process(
    const int16_t *input,
    int16_t *output,
    size_t num_samples,
    float pitch_factor);
