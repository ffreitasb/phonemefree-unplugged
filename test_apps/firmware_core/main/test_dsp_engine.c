#include "dsp_engine.h"

#include <stdatomic.h>

#include "unity.h"

TEST_CASE("dsp_engine init resets params and stats", "[dsp_engine]")
{
    atomic_store(&g_dsp_params.pitch_semitones, 7);
    atomic_store(&g_dsp_params.noise_level_pct, 42);
    atomic_store(&g_dsp_params.formant_shift_pct, -20);
    atomic_store(&g_dsp_params.dsp_enabled, false);

    TEST_ASSERT_EQUAL(ESP_OK, dsp_engine_init());

    TEST_ASSERT_EQUAL_INT(0, atomic_load(&g_dsp_params.pitch_semitones));
    TEST_ASSERT_EQUAL_INT(0, atomic_load(&g_dsp_params.noise_level_pct));
    TEST_ASSERT_EQUAL_INT(0, atomic_load(&g_dsp_params.formant_shift_pct));
    TEST_ASSERT_TRUE(atomic_load(&g_dsp_params.dsp_enabled));

    dsp_engine_stats_t stats = {0};
    dsp_engine_get_stats(&stats);

    TEST_ASSERT_EQUAL_UINT32(0, stats.samples_processed);
    TEST_ASSERT_EQUAL_UINT32(0, stats.input_underflows);
    TEST_ASSERT_EQUAL_UINT32(0, stats.output_drops);

    dsp_engine_get_stats(NULL);
}
