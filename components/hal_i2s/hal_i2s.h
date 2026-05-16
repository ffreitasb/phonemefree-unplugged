#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "freertos/ringbuf.h"

typedef struct {
    uint32_t samples_captured;
    uint32_t ringbuf_drops;
    uint32_t read_timeouts;
    uint32_t read_errors;
} hal_i2s_stats_t;

esp_err_t hal_i2s_init(RingbufHandle_t ringbuf_handle);
esp_err_t hal_i2s_start(void);
void hal_i2s_stop(void);
void hal_i2s_get_stats(hal_i2s_stats_t *stats);
