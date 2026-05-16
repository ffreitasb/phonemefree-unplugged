#include "hal_ringbuf.h"

#include <stdint.h>

#include "sdkconfig.h"

static RingbufHandle_t s_audio_ringbuf;
static RingbufHandle_t s_dsp_output_buf;

esp_err_t hal_ringbuf_init(void)
{
    if (s_audio_ringbuf && s_dsp_output_buf) {
        return ESP_OK;
    }

    s_audio_ringbuf = xRingbufferCreate(
        CONFIG_PHONEMEFREE_UNPLUGGED_RINGBUF_SAMPLES * sizeof(int16_t),
        RINGBUF_TYPE_BYTEBUF);
    if (!s_audio_ringbuf) {
        return ESP_ERR_NO_MEM;
    }

    s_dsp_output_buf = xRingbufferCreate(
        CONFIG_PHONEMEFREE_UNPLUGGED_DSP_OUTPUT_SAMPLES * sizeof(int16_t),
        RINGBUF_TYPE_BYTEBUF);
    if (!s_dsp_output_buf) {
        vRingbufferDelete(s_audio_ringbuf);
        s_audio_ringbuf = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

RingbufHandle_t hal_ringbuf_get_audio_input(void)
{
    return s_audio_ringbuf;
}

RingbufHandle_t hal_ringbuf_get_dsp_output(void)
{
    return s_dsp_output_buf;
}
