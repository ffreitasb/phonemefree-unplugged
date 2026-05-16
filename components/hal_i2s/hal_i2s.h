#pragma once

#include "esp_err.h"
#include "freertos/ringbuf.h"

esp_err_t hal_i2s_init(RingbufHandle_t ringbuf_handle);
void hal_i2s_start(void);
void hal_i2s_stop(void);
