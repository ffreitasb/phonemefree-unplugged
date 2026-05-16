idf_component_register(
    SRCS "dsp_engine.c"
    INCLUDE_DIRS "."
    REQUIRES dsp_noise dsp_pitch dsp_formant hal_ringbuf esp_common log
)
