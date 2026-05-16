#pragma once

#include "esp_err.h"

#define PHONEMEFREE_UNPLUGGED_WIFI_SSID "PhonemeFree Unplugged"

esp_err_t wifi_ap_init(void);
esp_err_t wifi_ap_start(void);
void wifi_ap_stop(void);
