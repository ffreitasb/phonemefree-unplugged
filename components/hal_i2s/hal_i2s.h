#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "freertos/ringbuf.h"

typedef struct {
    uint32_t samples_captured;
    uint32_t ringbuf_drops;
    uint32_t read_timeouts;
    uint32_t read_errors;
    int16_t last_min_sample;
    int16_t last_max_sample;
    uint16_t last_peak_abs;
} hal_i2s_stats_t;

typedef struct {
    uint32_t sample_count;
    int16_t min_sample;
    int16_t max_sample;
    uint16_t peak_abs;
    int32_t mean_sample;
    uint32_t rms;
} hal_i2s_level_stats_t;

esp_err_t hal_i2s_init(RingbufHandle_t ringbuf_handle);
esp_err_t hal_i2s_start(void);
void hal_i2s_stop(void);
void hal_i2s_get_stats(hal_i2s_stats_t *stats);
void hal_i2s_get_and_reset_level_stats(hal_i2s_level_stats_t *stats);
