#include "dsp_noise.h"

#include "esp_attr.h"

static uint32_t s_lfsr_state = 0xACE1u;

IRAM_ATTR static inline int16_t lfsr_next_sample(void)
{
    s_lfsr_state ^= s_lfsr_state << 13;
    s_lfsr_state ^= s_lfsr_state >> 17;
    s_lfsr_state ^= s_lfsr_state << 5;
    return (int16_t)(s_lfsr_state & 0xFFFF);
}

IRAM_ATTR static inline int16_t clamp_i16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }

    if (value < INT16_MIN) {
        return INT16_MIN;
    }

    return (int16_t)value;
}

IRAM_ATTR void dsp_noise_mix(int16_t *samples, size_t num_samples, uint8_t level_pct)
{
    if (!samples || num_samples == 0 || level_pct == 0) {
        return;
    }

    if (level_pct > 100) {
        level_pct = 100;
    }

    const int32_t signal_weight = 100 - level_pct;
    const int32_t noise_weight = level_pct;

    for (size_t i = 0; i < num_samples; ++i) {
        const int32_t signal = (int32_t)samples[i] * signal_weight;
        const int32_t noise = (int32_t)lfsr_next_sample() * noise_weight;
        samples[i] = clamp_i16((signal + noise) / 100);
    }
}
