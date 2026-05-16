#pragma once

#include <stddef.h>

#include "sdkconfig.h"

#if CONFIG_PHONEMEFREE_UNPLUGGED_FORMANT_ENABLE
void dsp_formant_apply_warp(float *fft_bins, size_t num_bins, float warp_factor);
#endif
