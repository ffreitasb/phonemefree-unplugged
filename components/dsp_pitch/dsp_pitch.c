#include "dsp_pitch.h"

#include <string.h>

#include "esp_attr.h"

IRAM_ATTR void dsp_pitch_process(
    const int16_t *input,
    int16_t *output,
    size_t num_samples,
    float pitch_factor)
{
    (void)pitch_factor;

    if (!input || !output || num_samples == 0) {
        return;
    }

    if (input == output) {
        return;
    }

    memcpy(output, input, num_samples * sizeof(int16_t));
}
