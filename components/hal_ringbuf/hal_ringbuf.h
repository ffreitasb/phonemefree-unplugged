#pragma once

#include "esp_err.h"
#include "freertos/ringbuf.h"

esp_err_t hal_ringbuf_init(void);
RingbufHandle_t hal_ringbuf_get_audio_input(void);
RingbufHandle_t hal_ringbuf_get_dsp_output(void);
