idf_component_register(
    SRCS "usb_audio_uac.c"
    INCLUDE_DIRS "."
    REQUIRES esp_common espressif__esp_tinyusb esp_ringbuf freertos hal_ringbuf log
)

set(usb_audio_defs
    CFG_TUD_AUDIO=0
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC ${usb_audio_defs})

idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
target_compile_definitions(${tinyusb_lib} PUBLIC ${usb_audio_defs})

idf_component_get_property(esp_tinyusb_lib espressif__esp_tinyusb COMPONENT_LIB)
target_compile_definitions(${esp_tinyusb_lib} PUBLIC ${usb_audio_defs})
