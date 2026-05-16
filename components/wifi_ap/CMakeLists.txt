idf_component_register(
    SRCS "wifi_ap.c"
    INCLUDE_DIRS "."
    REQUIRES esp_common esp_event esp_netif esp_wifi log nvs_flash
)
