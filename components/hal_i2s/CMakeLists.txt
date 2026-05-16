idf_component_register(
    SRCS "hal_i2s.c"
    INCLUDE_DIRS "."
    REQUIRES esp_common esp_driver_i2s esp_ringbuf freertos log
)
