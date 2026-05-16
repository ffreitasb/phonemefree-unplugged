#include "webserver_portal.h"

#include "esp_log.h"

static const char *TAG = "webserver_portal";

esp_err_t webserver_portal_init(void)
{
    ESP_LOGI(TAG, "Webserver portal scaffold initialized");
    return ESP_OK;
}

esp_err_t webserver_portal_start(void)
{
    ESP_LOGI(TAG, "Webserver portal not started in scaffold build");
    return ESP_OK;
}

void webserver_portal_stop(void)
{
}
