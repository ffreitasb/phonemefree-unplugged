#include "dsp_engine.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal_i2s.h"
#include "hal_ringbuf.h"
#include "sdkconfig.h"
#include "usb_audio_uac.h"
#include "webserver_portal.h"
#include "wifi_ap.h"

static const char *TAG = "phonemefree";

#if CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_LOG_ENABLE
#define BRINGUP_DBFS_FLOOR_CENTI (-12000)

static int32_t rms_to_dbfs_centi(uint32_t rms)
{
    if (rms == 0) {
        return BRINGUP_DBFS_FLOOR_CENTI;
    }

    const double dbfs = 20.0 * log10((double)rms / 32768.0);
    const double centi_dbfs = dbfs * 100.0;
    return (int32_t)(centi_dbfs < 0.0 ? centi_dbfs - 0.5 : centi_dbfs + 0.5);
}

static void format_centi_db(char *buffer, size_t buffer_size, int32_t centi_db)
{
    if (!buffer || buffer_size == 0) {
        return;
    }

    const char *sign = "";
    uint32_t magnitude = (uint32_t)centi_db;
    if (centi_db < 0) {
        sign = "-";
        magnitude = (uint32_t)(-centi_db);
    }

    (void)snprintf(
        buffer,
        buffer_size,
        "%s%" PRIu32 ".%02" PRIu32,
        sign,
        magnitude / 100,
        magnitude % 100);
}

static void bringup_log_task(void *arg)
{
    (void)arg;

    hal_i2s_stats_t i2s_stats = {0};
    hal_i2s_level_stats_t i2s_level = {0};
    dsp_engine_stats_t dsp_stats = {0};

    while (true) {
        hal_i2s_get_stats(&i2s_stats);
        hal_i2s_get_and_reset_level_stats(&i2s_level);
        dsp_engine_get_stats(&dsp_stats);

        char dbfs_buffer[16] = {0};
        format_centi_db(
            dbfs_buffer,
            sizeof(dbfs_buffer),
            rms_to_dbfs_centi(i2s_level.rms));

        ESP_LOGI(
            TAG,
            "bringup stats: i2s_samples=%" PRIu32
            " i2s_drops=%" PRIu32
            " i2s_timeouts=%" PRIu32
            " i2s_errors=%" PRIu32
            " i2s_window_samples=%" PRIu32
            " i2s_min=%d"
            " i2s_max=%d"
            " i2s_peak=%u"
            " i2s_mean=%" PRId32
            " i2s_rms=%" PRIu32
            " i2s_dbfs=%sdBFS"
            " dsp_samples=%" PRIu32
            " dsp_underflows=%" PRIu32
            " dsp_drops=%" PRIu32,
            i2s_stats.samples_captured,
            i2s_stats.ringbuf_drops,
            i2s_stats.read_timeouts,
            i2s_stats.read_errors,
            i2s_level.sample_count,
            (int)i2s_level.min_sample,
            (int)i2s_level.max_sample,
            (unsigned int)i2s_level.peak_abs,
            i2s_level.mean_sample,
            i2s_level.rms,
            dbfs_buffer,
            dsp_stats.samples_processed,
            dsp_stats.input_underflows,
            dsp_stats.output_drops);

        vTaskDelay(pdMS_TO_TICKS(CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_LOG_PERIOD_MS));
    }
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "PhonemeFree Unplugged booting");
    ESP_LOGI(TAG, "Reference build: ESP32-S3-WROOM-1 N8R8/N16R8 + ICS-43434, INMP441/MS3625 fallback");

#if CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_MODE
    ESP_LOGW(TAG, "Hardware bring-up mode enabled: USB Audio and Wi-Fi portal disabled");
#endif

    ESP_ERROR_CHECK(hal_ringbuf_init());
    ESP_ERROR_CHECK(hal_i2s_init(hal_ringbuf_get_audio_input()));
    ESP_ERROR_CHECK(dsp_engine_init());
#if !CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_MODE
    ESP_ERROR_CHECK(usb_audio_uac_init());
#else
    ESP_LOGW(TAG, "USB Audio disabled for hardware bring-up");
#endif

#if !CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_MODE
    ESP_ERROR_CHECK(wifi_ap_init());
    ESP_ERROR_CHECK(webserver_portal_init());
#else
    ESP_LOGW(TAG, "Wi-Fi portal disabled for hardware bring-up");
#endif

    ESP_ERROR_CHECK(dsp_engine_start());
    ESP_ERROR_CHECK(hal_i2s_start());
#if !CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_MODE
    ESP_ERROR_CHECK(usb_audio_uac_start());
#endif

#if !CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_MODE
    ESP_ERROR_CHECK(wifi_ap_start());
    ESP_ERROR_CHECK(webserver_portal_start());
#endif

#if CONFIG_PHONEMEFREE_UNPLUGGED_BRINGUP_LOG_ENABLE
    const BaseType_t bringup_log_created = xTaskCreate(
        bringup_log_task,
        "pf_bringup_log",
        3072,
        NULL,
        3,
        NULL);
    if (bringup_log_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bring-up log task");
    }
#endif

    ESP_LOGI(TAG, "Firmware services started");
}
