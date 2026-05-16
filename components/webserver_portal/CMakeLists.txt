idf_component_register(
    SRCS "webserver_portal.c"
    INCLUDE_DIRS "."
    REQUIRES dsp_engine esp_common esp_http_server hal_i2s joltwallet__littlefs log usb_audio_uac wifi_ap
)
