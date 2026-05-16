#include "wifi_ap.h"

#include <stdbool.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_ap";

static bool s_wifi_initialized;
static bool s_wifi_started;
static esp_netif_t *s_ap_netif;
static esp_event_handler_instance_t s_wifi_event_instance;

static esp_err_t ensure_nvs_ready(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Failed to erase NVS");
        err = nvs_flash_init();
    }

    return err;
}

static esp_err_t ensure_default_event_loop(void)
{
    esp_err_t err = esp_event_loop_create_default();
    return err == ESP_ERR_INVALID_STATE ? ESP_OK : err;
}

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    (void)arg;

    if (event_base != WIFI_EVENT) {
        return;
    }

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        const wifi_event_ap_staconnected_t *event = (const wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station connected, aid=%d", event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        const wifi_event_ap_stadisconnected_t *event = (const wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station disconnected, aid=%d", event->aid);
    }
}

esp_err_t wifi_ap_init(void)
{
    if (s_wifi_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(ensure_nvs_ready(), TAG, "Failed to initialize NVS");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize esp_netif");
    ESP_RETURN_ON_ERROR(ensure_default_event_loop(), TAG, "Failed to create event loop");

    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (!s_ap_netif) {
        return ESP_ERR_NO_MEM;
    }

    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_init_cfg), TAG, "Failed to initialize Wi-Fi");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "Failed to set Wi-Fi storage");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler,
            NULL,
            &s_wifi_event_instance),
        TAG,
        "Failed to register Wi-Fi event handler");

    wifi_config_t wifi_config = {0};
    snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", PHONEMEFREE_UNPLUGGED_WIFI_SSID);
    wifi_config.ap.ssid_len = strlen(PHONEMEFREE_UNPLUGGED_WIFI_SSID);
    wifi_config.ap.channel = PHONEMEFREE_UNPLUGGED_WIFI_CHANNEL;
    wifi_config.ap.max_connection = PHONEMEFREE_UNPLUGGED_WIFI_MAX_CONNECTIONS;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    wifi_config.ap.pmf_cfg.required = false;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "Failed to set Wi-Fi mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG, "Failed to set AP config");
    ESP_RETURN_ON_ERROR(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW20), TAG, "Failed to set AP bandwidth");

    s_wifi_initialized = true;
    ESP_LOGI(
        TAG,
        "Wi-Fi AP initialized: ssid=\"%s\" channel=%d max_conn=%d",
        PHONEMEFREE_UNPLUGGED_WIFI_SSID,
        PHONEMEFREE_UNPLUGGED_WIFI_CHANNEL,
        PHONEMEFREE_UNPLUGGED_WIFI_MAX_CONNECTIONS);
    return ESP_OK;
}

esp_err_t wifi_ap_start(void)
{
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_wifi_started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start Wi-Fi AP");
    s_wifi_started = true;
    ESP_LOGI(TAG, "Wi-Fi AP started at http://192.168.4.1/");
    return ESP_OK;
}

void wifi_ap_stop(void)
{
    if (!s_wifi_started) {
        return;
    }

    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to stop Wi-Fi cleanly: %s", esp_err_to_name(err));
        return;
    }

    s_wifi_started = false;
}
