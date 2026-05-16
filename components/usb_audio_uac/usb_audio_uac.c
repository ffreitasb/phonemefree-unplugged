#include "usb_audio_uac.h"

#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

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
#define USB_AUDIO_CHANNELS 1
#define USB_AUDIO_PACKET_SAMPLES (USB_AUDIO_SAMPLE_RATE_HZ / 1000)
#define USB_AUDIO_PACKET_BYTES (USB_AUDIO_PACKET_SAMPLES * USB_AUDIO_BYTES_PER_SAMPLE * USB_AUDIO_CHANNELS)
#define USB_AUDIO_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_AUDIO_MIC_ONE_CH_DESC_LEN)
#define USB_AUDIO_EP_IN 0x81
#define USB_AUDIO_FEEDER_TASK_STACK_SIZE 3072
#define USB_AUDIO_FEEDER_TASK_PRIORITY 6
#define USB_AUDIO_FEEDER_DELAY_TICKS pdMS_TO_TICKS(1)
#define USB_AUDIO_DISCONNECTED_DELAY_TICKS pdMS_TO_TICKS(10)
#define USB_AUDIO_TASK_CORE 0

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
    .bcdDevice = 0x0100,
    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t s_fs_configuration_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, USB_AUDIO_CONFIG_TOTAL_LEN, 0, 100),
    TUD_AUDIO_MIC_ONE_CH_DESCRIPTOR(
        ITF_NUM_AUDIO_CONTROL,
        STRID_AUDIO_INTERFACE,
        USB_AUDIO_BYTES_PER_SAMPLE,
        USB_AUDIO_BYTES_PER_SAMPLE * 8,
        USB_AUDIO_EP_IN,
        USB_AUDIO_PACKET_BYTES),
};

static const char s_langid[] = {0x09, 0x04};

static const char *s_string_desc[] = {
    s_langid,
    PHONEMEFREE_UNPLUGGED_USB_MANUFACTURER,
    PHONEMEFREE_UNPLUGGED_USB_PRODUCT,
    PHONEMEFREE_UNPLUGGED_USB_SERIAL,
    "UAC2 Microphone",
};

static atomic_bool s_driver_installed;
static atomic_bool s_feeder_running;
static atomic_bool s_usb_attached;
static atomic_uint s_packets_written;
static atomic_uint s_underruns;
static atomic_uint s_short_writes;
static TaskHandle_t s_feeder_task;

static bool s_mute[USB_AUDIO_CHANNELS + 1];
static int16_t s_volume[USB_AUDIO_CHANNELS + 1];
static uint32_t s_sample_freq;
static uint8_t s_clk_valid;
static audio_control_range_2_n_t(1) s_volume_range[USB_AUDIO_CHANNELS + 1];
static audio_control_range_4_n_t(1) s_sample_freq_range;

static void usb_audio_event_cb(tinyusb_event_t *event, void *arg)
{
    (void)arg;

    if (event->id == TINYUSB_EVENT_ATTACHED) {
        atomic_store(&s_usb_attached, true);
        ESP_LOGI(TAG, "USB attached on rhport %u", event->rhport);
    } else if (event->id == TINYUSB_EVENT_DETACHED) {
        atomic_store(&s_usb_attached, false);
        ESP_LOGI(TAG, "USB detached on rhport %u", event->rhport);
    }
}

static void usb_audio_init_controls(void)
{
    memset(s_mute, 0, sizeof(s_mute));
    memset(s_volume, 0, sizeof(s_volume));

    for (size_t i = 0; i < (USB_AUDIO_CHANNELS + 1); ++i) {
        s_volume_range[i].wNumSubRanges = 1;
        s_volume_range[i].subrange[0].bMin = 0;
        s_volume_range[i].subrange[0].bMax = 0;
        s_volume_range[i].subrange[0].bRes = 0;
    }

    s_sample_freq = USB_AUDIO_SAMPLE_RATE_HZ;
    s_clk_valid = 1;
    s_sample_freq_range.wNumSubRanges = 1;
    s_sample_freq_range.subrange[0].bMin = USB_AUDIO_SAMPLE_RATE_HZ;
    s_sample_freq_range.subrange[0].bMax = USB_AUDIO_SAMPLE_RATE_HZ;
    s_sample_freq_range.subrange[0].bRes = 0;
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

static void usb_audio_feeder_task(void *arg)
{
    (void)arg;

    RingbufHandle_t output_buf = hal_ringbuf_get_dsp_output();
    int16_t packet[USB_AUDIO_PACKET_SAMPLES];
    const int16_t *current_item = NULL;
    size_t current_bytes = 0;
    size_t current_offset = 0;

    while (atomic_load(&s_feeder_running)) {
        if (!tud_audio_mounted()) {
            if (current_item && output_buf) {
                vRingbufferReturnItem(output_buf, (void *)current_item);
                current_item = NULL;
                current_bytes = 0;
                current_offset = 0;
            }
            vTaskDelay(USB_AUDIO_DISCONNECTED_DELAY_TICKS);
            continue;
        }

        memset(packet, 0, sizeof(packet));
        size_t filled = 0;
        if (output_buf) {
            filled = usb_audio_fill_packet_from_ringbuf(
                output_buf,
                packet,
                USB_AUDIO_PACKET_SAMPLES,
                &current_item,
                &current_bytes,
                &current_offset);
        }

        if (filled < USB_AUDIO_PACKET_SAMPLES) {
            atomic_fetch_add(&s_underruns, 1);
        }

        const uint16_t written = tud_audio_write(packet, sizeof(packet));
        if (written == sizeof(packet)) {
            atomic_fetch_add(&s_packets_written, 1);
        } else {
            atomic_fetch_add(&s_short_writes, 1);
        }

        vTaskDelay(USB_AUDIO_FEEDER_DELAY_TICKS);
    }

    if (current_item && output_buf) {
        vRingbufferReturnItem(output_buf, (void *)current_item);
    }

    s_feeder_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t usb_audio_uac_init(void)
{
    if (atomic_load(&s_driver_installed)) {
        return ESP_OK;
    }

    usb_audio_init_controls();
    usb_audio_reset_stats();
    atomic_store(&s_usb_attached, false);

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
        "USB Audio UAC2 initialized (%u Hz, %u-bit mono)",
        USB_AUDIO_SAMPLE_RATE_HZ,
        USB_AUDIO_BYTES_PER_SAMPLE * 8);
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
                     tud_audio_mounted();
}

bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff)
{
    (void)rhport;
    (void)p_request;
    (void)pBuff;

    return false;
}

bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff)
{
    (void)rhport;
    (void)p_request;
    (void)pBuff;

    return false;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff)
{
    (void)rhport;

    const uint8_t channel_num = TU_U16_LOW(p_request->wValue);
    const uint8_t ctrl_sel = TU_U16_HIGH(p_request->wValue);
    const uint8_t entity_id = TU_U16_HIGH(p_request->wIndex);

    if (p_request->bRequest != AUDIO_CS_REQ_CUR || channel_num > USB_AUDIO_CHANNELS) {
        return false;
    }

    if (entity_id != 2) {
        return false;
    }

    switch (ctrl_sel) {
    case AUDIO_FU_CTRL_MUTE:
        if (p_request->wLength != sizeof(audio_control_cur_1_t)) {
            return false;
        }
        s_mute[channel_num] = ((const audio_control_cur_1_t *)pBuff)->bCur != 0;
        return true;

    case AUDIO_FU_CTRL_VOLUME:
        if (p_request->wLength != sizeof(audio_control_cur_2_t)) {
            return false;
        }
        s_volume[channel_num] = ((const audio_control_cur_2_t *)pBuff)->bCur;
        return true;

    default:
        return false;
    }
}

bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;
    (void)p_request;

    return false;
}

bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;
    (void)p_request;

    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    const uint8_t channel_num = TU_U16_LOW(p_request->wValue);
    const uint8_t ctrl_sel = TU_U16_HIGH(p_request->wValue);
    const uint8_t entity_id = TU_U16_HIGH(p_request->wIndex);

    if (channel_num > USB_AUDIO_CHANNELS) {
        return false;
    }

    switch (entity_id) {
    case 1:
        if (ctrl_sel == AUDIO_TE_CTRL_CONNECTOR) {
            const audio_desc_channel_cluster_t cluster = {
                .bNrChannels = USB_AUDIO_CHANNELS,
                .bmChannelConfig = AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
                .iChannelNames = 0,
            };
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport,
                p_request,
                (void *)&cluster,
                sizeof(cluster));
        }
        return false;

    case 2:
        switch (ctrl_sel) {
        case AUDIO_FU_CTRL_MUTE:
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport,
                p_request,
                &s_mute[channel_num],
                sizeof(s_mute[channel_num]));

        case AUDIO_FU_CTRL_VOLUME:
            if (p_request->bRequest == AUDIO_CS_REQ_CUR) {
                return tud_audio_buffer_and_schedule_control_xfer(
                    rhport,
                    p_request,
                    &s_volume[channel_num],
                    sizeof(s_volume[channel_num]));
            }
            if (p_request->bRequest == AUDIO_CS_REQ_RANGE) {
                return tud_audio_buffer_and_schedule_control_xfer(
                    rhport,
                    p_request,
                    &s_volume_range[channel_num],
                    sizeof(s_volume_range[channel_num]));
            }
            return false;

        default:
            return false;
        }

    case 4:
        switch (ctrl_sel) {
        case AUDIO_CS_CTRL_SAM_FREQ:
            if (p_request->bRequest == AUDIO_CS_REQ_CUR) {
                return tud_audio_buffer_and_schedule_control_xfer(
                    rhport,
                    p_request,
                    &s_sample_freq,
                    sizeof(s_sample_freq));
            }
            if (p_request->bRequest == AUDIO_CS_REQ_RANGE) {
                return tud_audio_buffer_and_schedule_control_xfer(
                    rhport,
                    p_request,
                    &s_sample_freq_range,
                    sizeof(s_sample_freq_range));
            }
            return false;

        case AUDIO_CS_CTRL_CLK_VALID:
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport,
                p_request,
                &s_clk_valid,
                sizeof(s_clk_valid));

        default:
            return false;
        }

    default:
        return false;
    }
}

bool tud_audio_set_itf_close_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;
    (void)p_request;

    return true;
}
