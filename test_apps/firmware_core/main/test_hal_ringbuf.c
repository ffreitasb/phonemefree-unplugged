#include "hal_ringbuf.h"

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "unity.h"

static void drain_ringbuf(RingbufHandle_t ringbuf)
{
    if (!ringbuf) {
        return;
    }

    for (;;) {
        size_t item_size = 0;
        void *item = xRingbufferReceive(ringbuf, &item_size, 0);
        if (!item) {
            break;
        }
        vRingbufferReturnItem(ringbuf, item);
    }
}

TEST_CASE("hal_ringbuf initializes idempotently", "[hal_ringbuf]")
{
    TEST_ASSERT_EQUAL(ESP_OK, hal_ringbuf_init());

    RingbufHandle_t audio_input = hal_ringbuf_get_audio_input();
    RingbufHandle_t dsp_output = hal_ringbuf_get_dsp_output();

    TEST_ASSERT_NOT_NULL(audio_input);
    TEST_ASSERT_NOT_NULL(dsp_output);

    TEST_ASSERT_EQUAL(ESP_OK, hal_ringbuf_init());
    TEST_ASSERT_TRUE(audio_input == hal_ringbuf_get_audio_input());
    TEST_ASSERT_TRUE(dsp_output == hal_ringbuf_get_dsp_output());
}

TEST_CASE("hal_ringbuf audio input stores and returns PCM frames", "[hal_ringbuf]")
{
    TEST_ASSERT_EQUAL(ESP_OK, hal_ringbuf_init());

    RingbufHandle_t audio_input = hal_ringbuf_get_audio_input();
    drain_ringbuf(audio_input);

    const int16_t frame[] = {100, -100, 200, -200, 0, 32767, -32768, 42};

    TEST_ASSERT_EQUAL(pdTRUE, xRingbufferSend(audio_input, frame, sizeof(frame), 0));

    size_t received_bytes = 0;
    void *received = xRingbufferReceive(audio_input, &received_bytes, 0);

    TEST_ASSERT_NOT_NULL(received);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)sizeof(frame), (uint32_t)received_bytes);
    TEST_ASSERT_EQUAL_MEMORY(frame, received, sizeof(frame));

    vRingbufferReturnItem(audio_input, received);
}

TEST_CASE("hal_ringbuf dsp output stores and returns USB frames", "[hal_ringbuf]")
{
    TEST_ASSERT_EQUAL(ESP_OK, hal_ringbuf_init());

    RingbufHandle_t dsp_output = hal_ringbuf_get_dsp_output();
    drain_ringbuf(dsp_output);

    const int16_t frame[] = {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
    };

    TEST_ASSERT_EQUAL(pdTRUE, xRingbufferSend(dsp_output, frame, sizeof(frame), 0));

    size_t received_bytes = 0;
    void *received = xRingbufferReceive(dsp_output, &received_bytes, 0);

    TEST_ASSERT_NOT_NULL(received);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)sizeof(frame), (uint32_t)received_bytes);
    TEST_ASSERT_EQUAL_MEMORY(frame, received, sizeof(frame));

    vRingbufferReturnItem(dsp_output, received);
}
