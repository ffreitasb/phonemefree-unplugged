#include "hal_i2s.h"

static RingbufHandle_t s_input_ringbuf;

esp_err_t hal_i2s_init(RingbufHandle_t ringbuf_handle)
{
    if (!ringbuf_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    s_input_ringbuf = ringbuf_handle;
    return ESP_OK;
}

void hal_i2s_start(void)
{
    (void)s_input_ringbuf;
}

void hal_i2s_stop(void)
{
}
