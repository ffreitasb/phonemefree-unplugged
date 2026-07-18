#include "usb_audio_uac.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "device/usbd_pvt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "hal_ringbuf.h"
#include "sdkconfig.h"
#include "tinyusb.h"
#include "tusb.h"

static const char *TAG = "usb_audio_uac";

#define USB_AUDIO_SAMPLE_RATE_HZ 16000
#define USB_AUDIO_BYTES_PER_SAMPLE 2
#define USB_AUDIO_BITS_PER_SAMPLE (USB_AUDIO_BYTES_PER_SAMPLE * 8)
#define USB_AUDIO_CHANNELS 1
#define USB_AUDIO_PACKET_SAMPLES (USB_AUDIO_SAMPLE_RATE_HZ / 1000)
#define USB_AUDIO_PACKET_BYTES (USB_AUDIO_PACKET_SAMPLES * USB_AUDIO_BYTES_PER_SAMPLE * USB_AUDIO_CHANNELS)
#define USB_AUDIO_EP_IN 0x81
#define USB_AUDIO_IAD_LEN 8
#define USB_AUDIO_AC_TOTAL_LEN 30
#define USB_AUDIO_UAC1_FUNCTION_LEN 91
#define USB_AUDIO_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + USB_AUDIO_IAD_LEN + USB_AUDIO_UAC1_FUNCTION_LEN)
#define USB_AUDIO_FEEDER_TASK_STACK_SIZE 3072
#define USB_AUDIO_FEEDER_TASK_PRIORITY 6
#define USB_AUDIO_FEEDER_DELAY_TICKS pdMS_TO_TICKS(1)
#define USB_AUDIO_DISCONNECTED_DELAY_TICKS pdMS_TO_TICKS(10)
#define USB_AUDIO_TASK_CORE 0

#define USB_AUDIO_UAC1_AUDIOCONTROL_VERSION 0x0100
#define USB_AUDIO_UAC1_TERMINAL_TYPE_USB_STREAMING 0x0101
#define USB_AUDIO_UAC1_TERMINAL_TYPE_MICROPHONE 0x0201
#define USB_AUDIO_UAC1_FORMAT_TAG_PCM 0x0001

#define U24_TO_U8S_LE(_value) \
    (uint8_t)((_value)&0xff), (uint8_t)(((_value) >> 8) & 0xff), (uint8_t)(((_value) >> 16) & 0xff)

enum {
    ITF_NUM_AUDIO_CONTROL = 0,
    ITF_NUM_AUDIO_STREAMING,
    ITF_NUM_TOTAL,
};

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_AUDIO_INTERFACE,
};

enum {
    USB_AUDIO_ENTITY_INPUT_TERMINAL = 1,
    USB_AUDIO_ENTITY_OUTPUT_TERMINAL = 2,
};

enum {
    USB_AUDIO_FUNCTION_SUBCLASS_UNDEFINED = 0x00,
    USB_AUDIO_SUBCLASS_CONTROL = 0x01,
    USB_AUDIO_SUBCLASS_STREAMING = 0x02,
    USB_AUDIO_PROTOCOL_UNDEFINED = 0x00,
};

enum {
    USB_AUDIO_CS_AC_HEADER = 0x01,
    USB_AUDIO_CS_AC_INPUT_TERMINAL = 0x02,
    USB_AUDIO_CS_AC_OUTPUT_TERMINAL = 0x03,
    USB_AUDIO_CS_AS_GENERAL = 0x01,
    USB_AUDIO_CS_AS_FORMAT_TYPE = 0x02,
    USB_AUDIO_FORMAT_TYPE_I = 0x01,
    USB_AUDIO_CS_ENDPOINT_GENERAL = 0x01,
};

static const tusb_desc_device_t s_device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = PHONEMEFREE_UNPLUGGED_USB_VID,
    .idProduct = PHONEMEFREE_UNPLUGGED_USB_PID,
    .bcdDevice = PHONEMEFREE_UNPLUGGED_USB_BCD_DEVICE,
    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t s_fs_configuration_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, USB_AUDIO_CONFIG_TOTAL_LEN, 0, 100),

    // Interface Association Descriptor for TinyUSB driver binding and Windows composite parsing.
    USB_AUDIO_IAD_LEN,
    TUSB_DESC_INTERFACE_ASSOCIATION,
    ITF_NUM_AUDIO_CONTROL,
    ITF_NUM_TOTAL,
    TUSB_CLASS_AUDIO,
    USB_AUDIO_FUNCTION_SUBCLASS_UNDEFINED,
    USB_AUDIO_PROTOCOL_UNDEFINED,
    STRID_AUDIO_INTERFACE,

    // Standard AC interface, UAC1 protocol.
    0x09,
    TUSB_DESC_INTERFACE,
    ITF_NUM_AUDIO_CONTROL,
    0x00,
    0x00,
    TUSB_CLASS_AUDIO,
    USB_AUDIO_SUBCLASS_CONTROL,
    USB_AUDIO_PROTOCOL_UNDEFINED,
    STRID_AUDIO_INTERFACE,

    // UAC1 class-specific AC header.
    0x09,
    TUSB_DESC_CS_INTERFACE,
    USB_AUDIO_CS_AC_HEADER,
    U16_TO_U8S_LE(USB_AUDIO_UAC1_AUDIOCONTROL_VERSION),
    U16_TO_U8S_LE(USB_AUDIO_AC_TOTAL_LEN),
    0x01,
    ITF_NUM_AUDIO_STREAMING,

    // UAC1 input terminal: mono microphone, no channel labels.
    0x0c,
    TUSB_DESC_CS_INTERFACE,
    USB_AUDIO_CS_AC_INPUT_TERMINAL,
    USB_AUDIO_ENTITY_INPUT_TERMINAL,
    U16_TO_U8S_LE(USB_AUDIO_UAC1_TERMINAL_TYPE_MICROPHONE),
    0x00,
    USB_AUDIO_CHANNELS,
    U16_TO_U8S_LE(0x0000),
    0x00,
    0x00,

    // UAC1 output terminal: USB streaming source linked directly to the mic terminal.
    0x09,
    TUSB_DESC_CS_INTERFACE,
    USB_AUDIO_CS_AC_OUTPUT_TERMINAL,
    USB_AUDIO_ENTITY_OUTPUT_TERMINAL,
    U16_TO_U8S_LE(USB_AUDIO_UAC1_TERMINAL_TYPE_USB_STREAMING),
    0x00,
    USB_AUDIO_ENTITY_INPUT_TERMINAL,
    0x00,

    // AudioStreaming alt setting 0: zero bandwidth.
    0x09,
    TUSB_DESC_INTERFACE,
    ITF_NUM_AUDIO_STREAMING,
    0x00,
    0x00,
    TUSB_CLASS_AUDIO,
    USB_AUDIO_SUBCLASS_STREAMING,
    USB_AUDIO_PROTOCOL_UNDEFINED,
    0x00,

    // AudioStreaming alt setting 1: mono PCM stream.
    0x09,
    TUSB_DESC_INTERFACE,
    ITF_NUM_AUDIO_STREAMING,
    0x01,
    0x01,
    TUSB_CLASS_AUDIO,
    USB_AUDIO_SUBCLASS_STREAMING,
    USB_AUDIO_PROTOCOL_UNDEFINED,
    0x00,

    // UAC1 AS general descriptor.
    0x07,
    TUSB_DESC_CS_INTERFACE,
    USB_AUDIO_CS_AS_GENERAL,
    USB_AUDIO_ENTITY_OUTPUT_TERMINAL,
    0x01,
    U16_TO_U8S_LE(USB_AUDIO_UAC1_FORMAT_TAG_PCM),

    // UAC1 type I format descriptor: mono, 16-bit, one discrete 16 kHz rate.
    0x0b,
    TUSB_DESC_CS_INTERFACE,
    USB_AUDIO_CS_AS_FORMAT_TYPE,
    USB_AUDIO_FORMAT_TYPE_I,
    USB_AUDIO_CHANNELS,
    USB_AUDIO_BYTES_PER_SAMPLE,
    USB_AUDIO_BITS_PER_SAMPLE,
    0x01,
    U24_TO_U8S_LE(USB_AUDIO_SAMPLE_RATE_HZ),

    // UAC1 standard isochronous audio data endpoint descriptor.
    0x09,
    TUSB_DESC_ENDPOINT,
    USB_AUDIO_EP_IN,
    (uint8_t)(TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA),
    U16_TO_U8S_LE(USB_AUDIO_PACKET_BYTES),
    0x01,
    0x00,
    0x00,

    // UAC1 class-specific isochronous audio data endpoint descriptor.
    0x07,
    TUSB_DESC_CS_ENDPOINT,
    USB_AUDIO_CS_ENDPOINT_GENERAL,
    0x00,
    0x00,
    U16_TO_U8S_LE(0x0000),
};

_Static_assert(USB_AUDIO_PACKET_BYTES == 32, "UAC1 endpoint packet size must match 16 samples/ms mono 16-bit");
_Static_assert(sizeof(s_fs_configuration_desc) == USB_AUDIO_CONFIG_TOTAL_LEN, "UAC1 config descriptor length mismatch");

static const char s_langid[] = {0x09, 0x04};

static const char *s_string_desc[] = {
    s_langid,
    PHONEMEFREE_UNPLUGGED_USB_MANUFACTURER,
    PHONEMEFREE_UNPLUGGED_USB_PRODUCT,
    PHONEMEFREE_UNPLUGGED_USB_SERIAL,
    "UAC1 Microphone",
};

static atomic_bool s_driver_installed;
static atomic_bool s_feeder_running;
static atomic_bool s_usb_attached;
static atomic_bool s_streaming;
static atomic_bool s_ep_busy;
static atomic_uint s_packets_written;
static atomic_uint s_underruns;
static atomic_uint s_short_writes;
static TaskHandle_t s_feeder_task;
static uint8_t s_audio_streaming_alt;
static uint8_t s_rhport;
static int16_t s_last_output_sample;

CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN static uint8_t s_usb_packet[USB_AUDIO_PACKET_BYTES];

static void usb_audio_event_cb(tinyusb_event_t *event, void *arg)
{
    (void)arg;

    if (event->id == TINYUSB_EVENT_ATTACHED) {
        atomic_store(&s_usb_attached, true);
        ESP_LOGI(TAG, "USB attached on rhport %u", event->rhport);
    } else if (event->id == TINYUSB_EVENT_DETACHED) {
        atomic_store(&s_usb_attached, false);
        atomic_store(&s_streaming, false);
        atomic_store(&s_ep_busy, false);
        s_audio_streaming_alt = 0;
        s_last_output_sample = 0;
        ESP_LOGI(TAG, "USB detached on rhport %u", event->rhport);
    }
}

static void usb_audio_reset_stats(void)
{
    atomic_store(&s_packets_written, 0);
    atomic_store(&s_underruns, 0);
    atomic_store(&s_short_writes, 0);
}

static size_t usb_audio_fill_packet_from_ringbuf(
    RingbufHandle_t output_buf,
    int16_t *packet,
    size_t packet_samples,
    const int16_t **current_item,
    size_t *current_bytes,
    size_t *current_offset)
{
    size_t filled = 0;

    while (filled < packet_samples) {
        if (*current_item == NULL) {
            *current_item = (const int16_t *)xRingbufferReceive(output_buf, current_bytes, 0);
            *current_offset = 0;
            if (*current_item == NULL) {
                break;
            }
        }

        const size_t available_bytes = *current_bytes - *current_offset;
        const size_t available_samples = available_bytes / sizeof(int16_t);
        const size_t wanted_samples = packet_samples - filled;
        const size_t samples_to_copy = available_samples < wanted_samples
                                           ? available_samples
                                           : wanted_samples;

        if (samples_to_copy > 0) {
            memcpy(
                &packet[filled],
                (const uint8_t *)(*current_item) + *current_offset,
                samples_to_copy * sizeof(int16_t));
            filled += samples_to_copy;
            *current_offset += samples_to_copy * sizeof(int16_t);
        }

        if (*current_offset >= *current_bytes || samples_to_copy == 0) {
            vRingbufferReturnItem(output_buf, (void *)*current_item);
            *current_item = NULL;
            *current_bytes = 0;
            *current_offset = 0;
        }
    }

    return filled;
}

static void usb_audio_smooth_underrun_tail(
    int16_t *packet,
    size_t filled_samples,
    size_t packet_samples)
{
    if (packet_samples == 0) {
        return;
    }

    if (filled_samples >= packet_samples) {
        s_last_output_sample = packet[packet_samples - 1];
        return;
    }

    const size_t tail_samples = packet_samples - filled_samples;
    const int32_t start_sample = filled_samples > 0
                                     ? packet[filled_samples - 1]
                                     : s_last_output_sample;

    for (size_t i = 0; i < tail_samples; ++i) {
        const int32_t numerator = (int32_t)(tail_samples - i - 1);
        packet[filled_samples + i] = (int16_t)((start_sample * numerator) / (int32_t)tail_samples);
    }

    s_last_output_sample = packet[packet_samples - 1];
}

static tusb_desc_endpoint_t const *usb_audio_find_ep_desc(void)
{
    const uint8_t *desc = s_fs_configuration_desc + TUD_CONFIG_DESC_LEN;
    const uint8_t *end = s_fs_configuration_desc + sizeof(s_fs_configuration_desc);

    while (desc < end && desc[0] != 0) {
        if (tu_desc_type(desc) == TUSB_DESC_ENDPOINT && desc[2] == USB_AUDIO_EP_IN) {
            return (tusb_desc_endpoint_t const *)desc;
        }
        desc = tu_desc_next(desc);
    }

    return NULL;
}

static void usb_audio_submit_packet(size_t filled_samples)
{
    if (!atomic_load(&s_streaming) || atomic_load(&s_ep_busy)) {
        return;
    }

    if (filled_samples < USB_AUDIO_PACKET_SAMPLES) {
        atomic_fetch_add(&s_underruns, 1);
    }

    if (!usbd_edpt_ready(s_rhport, USB_AUDIO_EP_IN)) {
        atomic_fetch_add(&s_short_writes, 1);
        return;
    }

    atomic_store(&s_ep_busy, true);
    if (!usbd_edpt_xfer(s_rhport, USB_AUDIO_EP_IN, s_usb_packet, sizeof(s_usb_packet))) {
        atomic_store(&s_ep_busy, false);
        atomic_fetch_add(&s_short_writes, 1);
    }
}

static void usb_audio_feeder_task(void *arg)
{
    (void)arg;

    RingbufHandle_t output_buf = hal_ringbuf_get_dsp_output();
    const int16_t *current_item = NULL;
    size_t current_bytes = 0;
    size_t current_offset = 0;

    while (atomic_load(&s_feeder_running)) {
        if (!atomic_load(&s_streaming)) {
            if (current_item && output_buf) {
                vRingbufferReturnItem(output_buf, (void *)current_item);
                current_item = NULL;
                current_bytes = 0;
                current_offset = 0;
            }
            vTaskDelay(USB_AUDIO_DISCONNECTED_DELAY_TICKS);
            continue;
        }

        if (!atomic_load(&s_ep_busy)) {
            memset(s_usb_packet, 0, sizeof(s_usb_packet));
            size_t filled = 0;
            if (output_buf) {
                filled = usb_audio_fill_packet_from_ringbuf(
                    output_buf,
                    (int16_t *)s_usb_packet,
                    USB_AUDIO_PACKET_SAMPLES,
                    &current_item,
                    &current_bytes,
                    &current_offset);
            }
            usb_audio_smooth_underrun_tail(
                (int16_t *)s_usb_packet,
                filled,
                USB_AUDIO_PACKET_SAMPLES);
            usb_audio_submit_packet(filled);
        }

        vTaskDelay(USB_AUDIO_FEEDER_DELAY_TICKS);
    }

    if (current_item && output_buf) {
        vRingbufferReturnItem(output_buf, (void *)current_item);
    }

    s_feeder_task = NULL;
    vTaskDelete(NULL);
}

static void usb_audio_uac1_init(void)
{
    atomic_store(&s_streaming, false);
    atomic_store(&s_ep_busy, false);
    s_audio_streaming_alt = 0;
    s_rhport = 0;
    s_last_output_sample = 0;
}

static bool usb_audio_uac1_deinit(void)
{
    atomic_store(&s_streaming, false);
    atomic_store(&s_ep_busy, false);
    s_audio_streaming_alt = 0;
    s_last_output_sample = 0;
    return true;
}

static void usb_audio_uac1_reset(uint8_t rhport)
{
    (void)rhport;

    atomic_store(&s_streaming, false);
    atomic_store(&s_ep_busy, false);
    s_audio_streaming_alt = 0;
    s_last_output_sample = 0;
}

static uint16_t usb_audio_uac1_open(
    uint8_t rhport,
    tusb_desc_interface_t const *desc_intf,
    uint16_t max_len)
{
    if (desc_intf->bInterfaceClass != TUSB_CLASS_AUDIO ||
        desc_intf->bInterfaceSubClass != USB_AUDIO_SUBCLASS_CONTROL ||
        desc_intf->bInterfaceProtocol != USB_AUDIO_PROTOCOL_UNDEFINED ||
        max_len < USB_AUDIO_UAC1_FUNCTION_LEN) {
        return 0;
    }

    s_rhport = rhport;
    s_audio_streaming_alt = 0;
    atomic_store(&s_streaming, false);
    atomic_store(&s_ep_busy, false);
    return USB_AUDIO_UAC1_FUNCTION_LEN;
}

static bool usb_audio_uac1_set_streaming_alt(uint8_t rhport, uint8_t alt)
{
    if (alt == 0) {
        atomic_store(&s_streaming, false);
        atomic_store(&s_ep_busy, false);
        usbd_edpt_close(rhport, USB_AUDIO_EP_IN);
        s_audio_streaming_alt = 0;
        s_last_output_sample = 0;
        return true;
    }

    if (alt != 1) {
        return false;
    }

    tusb_desc_endpoint_t const *ep_desc = usb_audio_find_ep_desc();
    if (!ep_desc) {
        return false;
    }

#ifdef TUP_DCD_EDPT_ISO_ALLOC
    if (!usbd_edpt_iso_activate(rhport, ep_desc)) {
        return false;
    }
#else
    if (!usbd_edpt_open(rhport, ep_desc)) {
        return false;
    }
#endif

    usbd_edpt_clear_stall(rhport, USB_AUDIO_EP_IN);
    s_audio_streaming_alt = 1;
    atomic_store(&s_ep_busy, false);
    atomic_store(&s_streaming, true);
    ESP_LOGI(TAG, "USB Audio UAC1 streaming alt setting enabled");
    return true;
}

static bool usb_audio_uac1_control_xfer_cb(
    uint8_t rhport,
    uint8_t stage,
    tusb_control_request_t const *request)
{
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    if (request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_INTERFACE) {
        return false;
    }

    const uint8_t interface = TU_U16_LOW(request->wIndex);
    if (interface != ITF_NUM_AUDIO_CONTROL && interface != ITF_NUM_AUDIO_STREAMING) {
        return false;
    }

    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_STANDARD) {
        return false;
    }

    switch (request->bRequest) {
    case TUSB_REQ_GET_INTERFACE:
        if (interface != ITF_NUM_AUDIO_STREAMING) {
            return false;
        }
        return tud_control_xfer(rhport, request, &s_audio_streaming_alt, sizeof(s_audio_streaming_alt));

    case TUSB_REQ_SET_INTERFACE:
        if (interface != ITF_NUM_AUDIO_STREAMING) {
            return false;
        }
        if (!usb_audio_uac1_set_streaming_alt(rhport, TU_U16_LOW(request->wValue))) {
            return false;
        }
        return tud_control_status(rhport, request);

    default:
        return false;
    }
}

static bool usb_audio_uac1_xfer_cb(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes)
{
    (void)rhport;

    if (ep_addr != USB_AUDIO_EP_IN) {
        return false;
    }

    atomic_store(&s_ep_busy, false);
    if (result == XFER_RESULT_SUCCESS && xferred_bytes == sizeof(s_usb_packet)) {
        atomic_fetch_add(&s_packets_written, 1);
    } else {
        atomic_fetch_add(&s_short_writes, 1);
    }

    return true;
}

static usbd_class_driver_t const s_uac1_driver = {
    .name = "pf_uac1",
    .init = usb_audio_uac1_init,
    .deinit = usb_audio_uac1_deinit,
    .reset = usb_audio_uac1_reset,
    .open = usb_audio_uac1_open,
    .control_xfer_cb = usb_audio_uac1_control_xfer_cb,
    .xfer_cb = usb_audio_uac1_xfer_cb,
    .xfer_isr = NULL,
    .sof = NULL,
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = 1;
    return &s_uac1_driver;
}

esp_err_t usb_audio_uac_init(void)
{
    if (atomic_load(&s_driver_installed)) {
        return ESP_OK;
    }

    usb_audio_reset_stats();
    atomic_store(&s_usb_attached, false);
    atomic_store(&s_streaming, false);
    atomic_store(&s_ep_busy, false);
    s_audio_streaming_alt = 0;
    s_last_output_sample = 0;

    const tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {
            .skip_setup = false,
            .self_powered = false,
            .vbus_monitor_io = -1,
        },
        .task = {
            .size = CONFIG_PHONEMEFREE_UNPLUGGED_USB_TASK_STACK_SIZE,
            .priority = 5,
            .xCoreID = USB_AUDIO_TASK_CORE,
        },
        .descriptor = {
            .device = &s_device_desc,
            .qualifier = NULL,
            .string = s_string_desc,
            .string_count = sizeof(s_string_desc) / sizeof(s_string_desc[0]),
            .full_speed_config = s_fs_configuration_desc,
            .high_speed_config = NULL,
        },
        .event_cb = usb_audio_event_cb,
        .event_arg = NULL,
    };

    const esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(err));
        return err;
    }

    atomic_store(&s_driver_installed, true);
    ESP_LOGI(
        TAG,
        "USB Audio UAC1 initialized (%u Hz, %u-bit mono)",
        USB_AUDIO_SAMPLE_RATE_HZ,
        USB_AUDIO_BITS_PER_SAMPLE);
    return ESP_OK;
}

esp_err_t usb_audio_uac_start(void)
{
    if (!atomic_load(&s_driver_installed)) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_feeder_task) {
        return ESP_OK;
    }

    atomic_store(&s_feeder_running, true);
    BaseType_t task_created = xTaskCreatePinnedToCore(
        usb_audio_feeder_task,
        "pf_usb_audio",
        USB_AUDIO_FEEDER_TASK_STACK_SIZE,
        NULL,
        USB_AUDIO_FEEDER_TASK_PRIORITY,
        &s_feeder_task,
        USB_AUDIO_TASK_CORE);

    if (task_created != pdPASS) {
        atomic_store(&s_feeder_running, false);
        s_feeder_task = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "USB Audio feeder task started");
    return ESP_OK;
}

void usb_audio_uac_stop(void)
{
    atomic_store(&s_feeder_running, false);
    for (int i = 0; s_feeder_task && i < 50; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (s_feeder_task) {
        ESP_LOGW(TAG, "USB Audio feeder task did not stop; leaving TinyUSB installed");
        return;
    }

    if (atomic_load(&s_driver_installed)) {
        esp_err_t err = tinyusb_driver_uninstall();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to uninstall TinyUSB driver: %s", esp_err_to_name(err));
        } else {
            atomic_store(&s_driver_installed, false);
            atomic_store(&s_usb_attached, false);
            atomic_store(&s_streaming, false);
            atomic_store(&s_ep_busy, false);
            s_audio_streaming_alt = 0;
            s_last_output_sample = 0;
        }
    }
}

void usb_audio_uac_get_stats(usb_audio_uac_stats_t *stats)
{
    if (!stats) {
        return;
    }

    stats->packets_written = atomic_load(&s_packets_written);
    stats->underruns = atomic_load(&s_underruns);
    stats->short_writes = atomic_load(&s_short_writes);
    stats->mounted = atomic_load(&s_driver_installed) &&
                     atomic_load(&s_usb_attached) &&
                     atomic_load(&s_streaming);
}
