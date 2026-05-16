#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    _Atomic int pitch_semitones;
    _Atomic int noise_level_pct;
    _Atomic int formant_shift_pct;
    _Atomic bool dsp_enabled;
} phonemefree_unplugged_dsp_params_t;

extern phonemefree_unplugged_dsp_params_t g_dsp_params;

typedef struct {
    uint32_t samples_processed;
    uint32_t input_underflows;
    uint32_t output_drops;
} dsp_engine_stats_t;

esp_err_t dsp_engine_init(void);
esp_err_t dsp_engine_start(void);
void dsp_engine_stop(void);
void dsp_engine_get_stats(dsp_engine_stats_t *stats);
