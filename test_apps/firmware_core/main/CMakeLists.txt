idf_component_register(
    SRCS
        "test_app_main.c"
        "test_dsp_engine.c"
        "test_dsp_noise.c"
        "test_hal_ringbuf.c"
    INCLUDE_DIRS "."
    REQUIRES unity dsp_engine dsp_noise hal_ringbuf
)
