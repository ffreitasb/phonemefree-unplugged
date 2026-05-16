#pragma once

#include "esp_err.h"

esp_err_t webserver_portal_init(void);
esp_err_t webserver_portal_start(void);
void webserver_portal_stop(void);
