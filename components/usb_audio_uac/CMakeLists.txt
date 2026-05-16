idf_component_register(
    SRCS "usb_audio_uac.c"
    INCLUDE_DIRS "."
    REQUIRES esp_common espressif__esp_tinyusb esp_ringbuf freertos hal_ringbuf log
)

set(usb_audio_defs
    CFG_TUD_AUDIO=1
    CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE=16000
    CFG_TUD_AUDIO_FUNC_1_DESC_LEN=TUD_AUDIO_MIC_ONE_CH_DESC_LEN
    CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ=64
    CFG_TUD_AUDIO_ENABLE_EP_IN=1
    CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX=2
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX=1
    CFG_TUD_AUDIO_EP_SZ_IN=32
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX=32
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ=256
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC ${usb_audio_defs})

idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
target_compile_definitions(${tinyusb_lib} PUBLIC ${usb_audio_defs})

idf_component_get_property(esp_tinyusb_lib espressif__esp_tinyusb COMPONENT_LIB)
target_compile_definitions(${esp_tinyusb_lib} PUBLIC ${usb_audio_defs})
