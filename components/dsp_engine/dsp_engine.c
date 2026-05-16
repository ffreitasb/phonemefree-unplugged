#include "dsp_engine.h"

#include <stdatomic.h>

#include "esp_log.h"

static const char *TAG = "dsp_engine";

phonemefree_unplugged_dsp_params_t g_dsp_params;

esp_err_t dsp_engine_init(void)
{
    atomic_store(&g_dsp_params.pitch_semitones, 0);
    atomic_store(&g_dsp_params.noise_level_pct, 0);
    atomic_store(&g_dsp_params.formant_shift_pct, 0);
    atomic_store(&g_dsp_params.dsp_enabled, true);

    ESP_LOGI(TAG, "DSP parameter state initialized");
    return ESP_OK;
}

esp_err_t dsp_engine_start(void)
{
    ESP_LOGI(TAG, "DSP engine task not started in scaffold build");
    return ESP_OK;
}

void dsp_engine_stop(void)
{
}
