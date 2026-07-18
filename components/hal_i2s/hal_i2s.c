#include "hal_i2s.h"

#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define HAL_I2S_SAMPLE_RATE_HZ 16000
#define HAL_I2S_CAPTURE_SAMPLES 64
#define HAL_I2S_READ_TIMEOUT_MS 100
#define HAL_I2S_TASK_PRIORITY 6
#if CONFIG_FREERTOS_UNICORE
#define HAL_I2S_TASK_CORE 0
#else
#define HAL_I2S_TASK_CORE 1
#endif

static const char *TAG = "hal_i2s";

static RingbufHandle_t s_input_ringbuf;
static i2s_chan_handle_t s_rx_channel;
static TaskHandle_t s_capture_task;

static atomic_bool s_capture_running;
static atomic_uint s_samples_captured;
static atomic_uint s_ringbuf_drops;
static atomic_uint s_read_timeouts;
static atomic_uint s_read_errors;
static atomic_int s_last_min_sample;
static atomic_int s_last_max_sample;
static atomic_uint s_last_peak_abs;

typedef struct {
    uint32_t sample_count;
    int64_t sample_sum;
    uint64_t sample_square_sum;
    int16_t min_sample;
    int16_t max_sample;
    uint16_t peak_abs;
} level_accumulator_t;

static portMUX_TYPE s_level_stats_mux = portMUX_INITIALIZER_UNLOCKED;
static level_accumulator_t s_level_window = {
    .min_sample = INT16_MAX,
    .max_sample = INT16_MIN,
};

static inline int16_t inmp441_frame_to_pcm16(int32_t frame)
{
    return (int16_t)(frame >> 16);
}

static void reset_level_accumulator(level_accumulator_t *accumulator)
{
    accumulator->sample_count = 0;
    accumulator->sample_sum = 0;
    accumulator->sample_square_sum = 0;
    accumulator->min_sample = INT16_MAX;
    accumulator->max_sample = INT16_MIN;
    accumulator->peak_abs = 0;
}

static uint64_t integer_sqrt_u64(uint64_t value)
{
    uint64_t result = 0;
    uint64_t bit = 1ULL << 62;

    while (bit > value) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }

    return result;
}

static void update_level_stats(const int16_t *samples, size_t sample_count)
{
    int16_t min_sample = INT16_MAX;
    int16_t max_sample = INT16_MIN;
    uint16_t peak_abs = 0;
    int64_t sample_sum = 0;
    uint64_t sample_square_sum = 0;

    for (size_t i = 0; i < sample_count; ++i) {
        const int16_t sample = samples[i];
        const int32_t sample_32 = (int32_t)sample;

        sample_sum += sample_32;
        sample_square_sum += (uint64_t)(sample_32 * sample_32);

        if (sample < min_sample) {
            min_sample = sample;
        }
        if (sample > max_sample) {
            max_sample = sample;
        }

        const uint16_t abs_sample = sample == INT16_MIN
                                        ? (uint16_t)INT16_MAX + 1U
                                        : (uint16_t)(sample < 0 ? -sample : sample);
        if (abs_sample > peak_abs) {
            peak_abs = abs_sample;
        }
    }

    atomic_store(&s_last_min_sample, (int)min_sample);
    atomic_store(&s_last_max_sample, (int)max_sample);
    atomic_store(&s_last_peak_abs, (unsigned int)peak_abs);

    portENTER_CRITICAL(&s_level_stats_mux);
    s_level_window.sample_count += (uint32_t)sample_count;
    s_level_window.sample_sum += sample_sum;
    s_level_window.sample_square_sum += sample_square_sum;
    if (min_sample < s_level_window.min_sample) {
        s_level_window.min_sample = min_sample;
    }
    if (max_sample > s_level_window.max_sample) {
        s_level_window.max_sample = max_sample;
    }
    if (peak_abs > s_level_window.peak_abs) {
        s_level_window.peak_abs = peak_abs;
    }
    portEXIT_CRITICAL(&s_level_stats_mux);
}

static void hal_i2s_capture_task(void *arg)
{
    (void)arg;

    int32_t raw_frames[HAL_I2S_CAPTURE_SAMPLES];
    int16_t pcm_samples[HAL_I2S_CAPTURE_SAMPLES];

    while (atomic_load(&s_capture_running)) {
        size_t bytes_read = 0;
        esp_err_t ret = i2s_channel_read(
            s_rx_channel,
            raw_frames,
            sizeof(raw_frames),
            &bytes_read,
            HAL_I2S_READ_TIMEOUT_MS);

        if (ret == ESP_ERR_TIMEOUT) {
            atomic_fetch_add(&s_read_timeouts, 1);
            continue;
        }

        if (ret != ESP_OK) {
            atomic_fetch_add(&s_read_errors, 1);
            continue;
        }

        const size_t frame_count = bytes_read / sizeof(raw_frames[0]);
        if (frame_count == 0) {
            continue;
        }

        for (size_t i = 0; i < frame_count; ++i) {
            pcm_samples[i] = inmp441_frame_to_pcm16(raw_frames[i]);
        }
        update_level_stats(pcm_samples, frame_count);

        const size_t pcm_bytes = frame_count * sizeof(pcm_samples[0]);
        if (xRingbufferSend(s_input_ringbuf, pcm_samples, pcm_bytes, 0) != pdTRUE) {
            atomic_fetch_add(&s_ringbuf_drops, 1);
            continue;
        }

        atomic_fetch_add(&s_samples_captured, (unsigned int)frame_count);
    }

    s_capture_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t hal_i2s_init(RingbufHandle_t ringbuf_handle)
{
    if (!ringbuf_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_rx_channel) {
        s_input_ringbuf = ringbuf_handle;
        return ESP_OK;
    }

    s_input_ringbuf = ringbuf_handle;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        CONFIG_PHONEMEFREE_UNPLUGGED_I2S_PORT,
        I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = HAL_I2S_CAPTURE_SAMPLES;

    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &s_rx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to allocate I2S RX channel: %s", esp_err_to_name(ret));
        s_rx_channel = NULL;
        return ret;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(HAL_I2S_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT,
            I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)CONFIG_PHONEMEFREE_UNPLUGGED_I2S_BCK_PIN,
            .ws = (gpio_num_t)CONFIG_PHONEMEFREE_UNPLUGGED_I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = (gpio_num_t)CONFIG_PHONEMEFREE_UNPLUGGED_I2S_DATA_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;

    ret = i2s_channel_init_std_mode(s_rx_channel, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S STD RX: %s", esp_err_to_name(ret));
        (void)i2s_del_channel(s_rx_channel);
        s_rx_channel = NULL;
        return ret;
    }

    ESP_LOGI(
        TAG,
        "I2S RX ready: port=%d bck=%d ws=%d din=%d sample_rate=%d",
        CONFIG_PHONEMEFREE_UNPLUGGED_I2S_PORT,
        CONFIG_PHONEMEFREE_UNPLUGGED_I2S_BCK_PIN,
        CONFIG_PHONEMEFREE_UNPLUGGED_I2S_WS_PIN,
        CONFIG_PHONEMEFREE_UNPLUGGED_I2S_DATA_PIN,
        HAL_I2S_SAMPLE_RATE_HZ);

    return ESP_OK;
}

esp_err_t hal_i2s_start(void)
{
    if (!s_rx_channel || !s_input_ringbuf) {
        return ESP_ERR_INVALID_STATE;
    }

    if (atomic_load(&s_capture_running)) {
        return ESP_OK;
    }

    esp_err_t ret = i2s_channel_enable(s_rx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S RX: %s", esp_err_to_name(ret));
        return ret;
    }

    atomic_store(&s_capture_running, true);
    if (xTaskCreatePinnedToCore(
            hal_i2s_capture_task,
            "pf_i2s_capture",
            CONFIG_PHONEMEFREE_UNPLUGGED_I2S_TASK_STACK_SIZE,
            NULL,
            HAL_I2S_TASK_PRIORITY,
            &s_capture_task,
            HAL_I2S_TASK_CORE) != pdPASS) {
        atomic_store(&s_capture_running, false);
        (void)i2s_channel_disable(s_rx_channel);
        s_capture_task = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "I2S capture started");
    return ESP_OK;
}

void hal_i2s_stop(void)
{
    atomic_store(&s_capture_running, false);

    for (int i = 0; i < 20 && s_capture_task; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (s_rx_channel) {
        esp_err_t ret = i2s_channel_disable(s_rx_channel);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "Failed to disable I2S RX cleanly: %s", esp_err_to_name(ret));
        }
    }
}

void hal_i2s_get_stats(hal_i2s_stats_t *stats)
{
    if (!stats) {
        return;
    }

    stats->samples_captured = atomic_load(&s_samples_captured);
    stats->ringbuf_drops = atomic_load(&s_ringbuf_drops);
    stats->read_timeouts = atomic_load(&s_read_timeouts);
    stats->read_errors = atomic_load(&s_read_errors);
    stats->last_min_sample = (int16_t)atomic_load(&s_last_min_sample);
    stats->last_max_sample = (int16_t)atomic_load(&s_last_max_sample);
    stats->last_peak_abs = (uint16_t)atomic_load(&s_last_peak_abs);
}

void hal_i2s_get_and_reset_level_stats(hal_i2s_level_stats_t *stats)
{
    if (!stats) {
        return;
    }

    level_accumulator_t snapshot;

    portENTER_CRITICAL(&s_level_stats_mux);
    snapshot = s_level_window;
    reset_level_accumulator(&s_level_window);
    portEXIT_CRITICAL(&s_level_stats_mux);

    if (snapshot.sample_count == 0) {
        *stats = (hal_i2s_level_stats_t){0};
        return;
    }

    const uint64_t mean_square = snapshot.sample_square_sum / snapshot.sample_count;

    stats->sample_count = snapshot.sample_count;
    stats->min_sample = snapshot.min_sample;
    stats->max_sample = snapshot.max_sample;
    stats->peak_abs = snapshot.peak_abs;
    stats->mean_sample = (int32_t)(snapshot.sample_sum / snapshot.sample_count);
    stats->rms = (uint32_t)integer_sqrt_u64(mean_square);
}
