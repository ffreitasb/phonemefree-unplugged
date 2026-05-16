#pragma once

#include "esp_err.h"

#define PHONEMEFREE_UNPLUGGED_USB_VID 0x303A
#define PHONEMEFREE_UNPLUGGED_USB_PID 0x4001
#define PHONEMEFREE_UNPLUGGED_USB_MANUFACTURER "PhonemeFree Unplugged"
#define PHONEMEFREE_UNPLUGGED_USB_PRODUCT "PhonemeFree Unplugged Mic"
#define PHONEMEFREE_UNPLUGGED_USB_SERIAL "001"

esp_err_t usb_audio_uac_init(void);
esp_err_t usb_audio_uac_start(void);
void usb_audio_uac_stop(void);
