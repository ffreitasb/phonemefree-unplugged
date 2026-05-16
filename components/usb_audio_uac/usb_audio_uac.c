#include "usb_audio_uac.h"

#include "esp_log.h"

static const char *TAG = "usb_audio_uac";

esp_err_t usb_audio_uac_init(void)
{
    ESP_LOGI(TAG, "USB Audio scaffold initialized");
    return ESP_OK;
}

esp_err_t usb_audio_uac_start(void)
{
    ESP_LOGI(TAG, "USB Audio task not started in scaffold build");
    return ESP_OK;
}

void usb_audio_uac_stop(void)
{
}
