#include "dsp_engine.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal_i2s.h"
#include "hal_ringbuf.h"
#include "usb_audio_uac.h"
#include "webserver_portal.h"
#include "wifi_ap.h"

static const char *TAG = "phonemefree";

void app_main(void)
{
    ESP_LOGI(TAG, "PhonemeFree Unplugged booting");
    ESP_LOGI(TAG, "Reference build: ESP32-S3-WROOM-1 N8R8/N16R8 + INMP441");

    ESP_ERROR_CHECK(hal_ringbuf_init());
    ESP_ERROR_CHECK(hal_i2s_init(hal_ringbuf_get_audio_input()));
    ESP_ERROR_CHECK(dsp_engine_init());
    ESP_ERROR_CHECK(usb_audio_uac_init());
    ESP_ERROR_CHECK(wifi_ap_init());
    ESP_ERROR_CHECK(webserver_portal_init());

    ESP_ERROR_CHECK(dsp_engine_start());
    ESP_ERROR_CHECK(hal_i2s_start());
    ESP_ERROR_CHECK(usb_audio_uac_start());
    ESP_ERROR_CHECK(wifi_ap_start());
    ESP_ERROR_CHECK(webserver_portal_start());

    ESP_LOGI(TAG, "Firmware services started");
}
