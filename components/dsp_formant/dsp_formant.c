#include "dsp_formant.h"

#include "esp_attr.h"

#if CONFIG_PHONEMEFREE_UNPLUGGED_FORMANT_ENABLE
IRAM_ATTR void dsp_formant_apply_warp(float *fft_bins, size_t num_bins, float warp_factor)
{
    (void)fft_bins;
    (void)num_bins;
    (void)warp_factor;
}
#endif
