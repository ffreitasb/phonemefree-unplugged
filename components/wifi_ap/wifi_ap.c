#include "wifi_ap.h"

#include "esp_log.h"

static const char *TAG = "wifi_ap";

esp_err_t wifi_ap_init(void)
{
    ESP_LOGI(TAG, "Wi-Fi AP scaffold initialized for SSID \"%s\"", PHONEMEFREE_UNPLUGGED_WIFI_SSID);
    return ESP_OK;
}

esp_err_t wifi_ap_start(void)
{
    ESP_LOGI(TAG, "Wi-Fi AP not started in scaffold build");
    return ESP_OK;
}

void wifi_ap_stop(void)
{
}
