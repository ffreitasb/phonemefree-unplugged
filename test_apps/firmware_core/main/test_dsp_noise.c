#include "dsp_noise.h"

#include <stddef.h>
#include <stdint.h>

#include "unity.h"

TEST_CASE("dsp_noise leaves samples unchanged when disabled", "[dsp_noise]")
{
    int16_t samples[] = {-32768, -42, 0, 42, 32767};
    const int16_t expected[] = {-32768, -42, 0, 42, 32767};

    dsp_noise_reset(DSP_NOISE_DEFAULT_SEED);
    dsp_noise_mix(samples, sizeof(samples) / sizeof(samples[0]), 0);

    TEST_ASSERT_EQUAL_INT16_ARRAY(expected, samples, sizeof(samples) / sizeof(samples[0]));

    int16_t probe[] = {0};
    dsp_noise_mix(probe, 1, 100);
    TEST_ASSERT_EQUAL_INT16(17359, probe[0]);
}

TEST_CASE("dsp_noise emits deterministic full-level noise", "[dsp_noise]")
{
    int16_t samples[] = {0, 0, 0, 0};
    const int16_t expected[] = {17359, 10837, -9551, 17666};

    dsp_noise_reset(DSP_NOISE_DEFAULT_SEED);
    dsp_noise_mix(samples, sizeof(samples) / sizeof(samples[0]), 100);

    TEST_ASSERT_EQUAL_INT16_ARRAY(expected, samples, sizeof(samples) / sizeof(samples[0]));
}

TEST_CASE("dsp_noise clamps levels above 100 percent", "[dsp_noise]")
{
    int16_t samples[] = {0, 0};
    const int16_t expected[] = {17359, 10837};

    dsp_noise_reset(DSP_NOISE_DEFAULT_SEED);
    dsp_noise_mix(samples, sizeof(samples) / sizeof(samples[0]), 250);

    TEST_ASSERT_EQUAL_INT16_ARRAY(expected, samples, sizeof(samples) / sizeof(samples[0]));
}

TEST_CASE("dsp_noise mixes signal and deterministic noise linearly", "[dsp_noise]")
{
    int16_t samples[] = {1000, -1000, 30000, -30000};
    const int16_t expected[] = {9179, 4918, 10224, -6167};

    dsp_noise_reset(DSP_NOISE_DEFAULT_SEED);
    dsp_noise_mix(samples, sizeof(samples) / sizeof(samples[0]), 50);

    TEST_ASSERT_EQUAL_INT16_ARRAY(expected, samples, sizeof(samples) / sizeof(samples[0]));
}

TEST_CASE("dsp_noise tolerates empty inputs", "[dsp_noise]")
{
    int16_t sample = 1234;

    dsp_noise_mix(NULL, 4, 100);
    dsp_noise_mix(&sample, 0, 100);

    TEST_ASSERT_EQUAL_INT16(1234, sample);
}
