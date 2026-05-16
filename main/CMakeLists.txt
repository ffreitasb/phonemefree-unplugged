idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_common
        log
        hal_i2s
        hal_ringbuf
        dsp_engine
        usb_audio_uac
        wifi_ap
        webserver_portal
)
