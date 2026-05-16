#include "dsp_engine.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dsp_noise.h"
#include "dsp_pitch.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "hal_ringbuf.h"
#include "sdkconfig.h"

#define DSP_ENGINE_FRAME_SAMPLES 64
#define DSP_ENGINE_RECEIVE_TIMEOUT_TICKS pdMS_TO_TICKS(20)
#define DSP_ENGINE_TASK_PRIORITY 5
#if CONFIG_FREERTOS_UNICORE
#define DSP_ENGINE_TASK_CORE 0
#else
#define DSP_ENGINE_TASK_CORE 1
#endif

static const char *TAG = "dsp_engine";

phonemefree_unplugged_dsp_params_t g_dsp_params;

static TaskHandle_t s_dsp_task;
static atomic_bool s_dsp_running;
static atomic_uint s_samples_processed;
static atomic_uint s_input_underflows;
static atomic_uint s_output_drops;

static void process_chunk(const int16_t *input, int16_t *output, size_t sample_count)
{
    if (!atomic_load(&g_dsp_params.dsp_enabled)) {
        memcpy(output, input, sample_count * sizeof(output[0]));
        return;
    }

    const int pitch_semitones = atomic_load(&g_dsp_params.pitch_semitones);
    int noise_level_pct = atomic_load(&g_dsp_params.noise_level_pct);
    if (noise_level_pct < 0) {
        noise_level_pct = 0;
    } else if (noise_level_pct > 100) {
        noise_level_pct = 100;
    }

    (void)pitch_semitones;
    dsp_pitch_process(input, output, sample_count, 1.0f);

    if (noise_level_pct > 0) {
        dsp_noise_mix(output, sample_count, (uint8_t)noise_level_pct);
    }
}

static void dsp_engine_task(void *arg)
{
    (void)arg;

    RingbufHandle_t input_ringbuf = hal_ringbuf_get_audio_input();
    RingbufHandle_t output_ringbuf = hal_ringbuf_get_dsp_output();
    int16_t output_frame[DSP_ENGINE_FRAME_SAMPLES];

    while (atomic_load(&s_dsp_running)) {
        size_t item_bytes = 0;
        void *item = xRingbufferReceive(
            input_ringbuf,
            &item_bytes,
            DSP_ENGINE_RECEIVE_TIMEOUT_TICKS);

        if (!item) {
            atomic_fetch_add(&s_input_underflows, 1);
            continue;
        }

        const int16_t *input_samples = (const int16_t *)item;
        size_t remaining_samples = item_bytes / sizeof(input_samples[0]);
        size_t offset = 0;

        while (remaining_samples > 0) {
            const size_t chunk_samples =
                remaining_samples > DSP_ENGINE_FRAME_SAMPLES
                    ? DSP_ENGINE_FRAME_SAMPLES
                    : remaining_samples;

            process_chunk(&input_samples[offset], output_frame, chunk_samples);

            const size_t output_bytes = chunk_samples * sizeof(output_frame[0]);
            if (xRingbufferSend(output_ringbuf, output_frame, output_bytes, 0) != pdTRUE) {
                atomic_fetch_add(&s_output_drops, 1);
            } else {
                atomic_fetch_add(&s_samples_processed, (unsigned int)chunk_samples);
            }

            offset += chunk_samples;
            remaining_samples -= chunk_samples;
        }

        vRingbufferReturnItem(input_ringbuf, item);
    }

    s_dsp_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t dsp_engine_init(void)
{
    atomic_store(&g_dsp_params.pitch_semitones, 0);
    atomic_store(&g_dsp_params.noise_level_pct, 0);
    atomic_store(&g_dsp_params.formant_shift_pct, 0);
    atomic_store(&g_dsp_params.dsp_enabled, true);

    atomic_store(&s_samples_processed, 0);
    atomic_store(&s_input_underflows, 0);
    atomic_store(&s_output_drops, 0);
    dsp_noise_reset(DSP_NOISE_DEFAULT_SEED);

    ESP_LOGI(TAG, "DSP parameter state initialized");
    return ESP_OK;
}

esp_err_t dsp_engine_start(void)
{
    if (atomic_load(&s_dsp_running)) {
        return ESP_OK;
    }

    if (!hal_ringbuf_get_audio_input() || !hal_ringbuf_get_dsp_output()) {
        return ESP_ERR_INVALID_STATE;
    }

    atomic_store(&s_dsp_running, true);
    if (xTaskCreatePinnedToCore(
            dsp_engine_task,
            "pf_dsp",
            CONFIG_PHONEMEFREE_UNPLUGGED_DSP_TASK_STACK_SIZE,
            NULL,
            DSP_ENGINE_TASK_PRIORITY,
            &s_dsp_task,
            DSP_ENGINE_TASK_CORE) != pdPASS) {
        atomic_store(&s_dsp_running, false);
        s_dsp_task = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "DSP engine started");
    return ESP_OK;
}

void dsp_engine_stop(void)
{
    atomic_store(&s_dsp_running, false);

    for (int i = 0; i < 20 && s_dsp_task; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void dsp_engine_get_stats(dsp_engine_stats_t *stats)
{
    if (!stats) {
        return;
    }

    stats->samples_processed = atomic_load(&s_samples_processed);
    stats->input_underflows = atomic_load(&s_input_underflows);
    stats->output_drops = atomic_load(&s_output_drops);
}
