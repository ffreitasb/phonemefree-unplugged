# Reference Build v0.1 - PhonemeFree Unplugged

This document locks the reference hardware build for the first functional MVP.

The reference build is the hardware target that firmware development should optimize for first. Other ESP32-S3 boards may work, but they should not shape the v0.1 firmware until this build is stable.

## 1. Reference Build

| Area | Reference Choice | Reason |
| --- | --- | --- |
| MCU board | ESP32-S3-WROOM-1 `N8R8` or `N16R8` USB-C development board | Full-size board, exposed pins, native USB, PSRAM headroom, common AliExpress availability |
| Microphone | INMP441 I2S microphone breakout module | Common, cheap, PRD-aligned, 24-bit I2S output |
| Prototype medium | Breadboard first, then double-sided 2.54 mm perfboard | Fast bring-up first, sturdier DIY build after proof |
| USB connection | Native ESP32-S3 USB port to host | Required for USB Audio Class microphone mode |
| Power | 5 V from USB host, onboard 3.3 V regulator for ESP32-S3 and mic | Simplest public MVP power path |
| Firmware framework | ESP-IDF | Primary development flow; release flashing UX comes later |

Recommended search terms:

- `ESP32-S3 WROOM N16R8 USB C development board`
- `ESP32-S3 WROOM N8R8 USB C development board`
- `ESP32-S3 DevKitC-1 N8R8`
- `INMP441 I2S microphone module ESP32`
- `double side prototype PCB 2.54mm`

## 2. Fixed v0.1 Pinout

The v0.1 firmware should assume this pinout by default:

| Signal | ESP32-S3 GPIO | INMP441-style Module Pin |
| --- | ---: | --- |
| I2S bit clock | GPIO4 | `SCK` / `BCLK` |
| I2S word select | GPIO5 | `WS` / `LRCLK` |
| I2S data input | GPIO6 | `SD` / `DOUT` |
| Microphone power | 3V3 | `VDD` / `3V` |
| Ground | GND | `GND` |
| Channel select | GND first | `L/R` |
| USB D- | GPIO19 | Native USB connector |
| USB D+ | GPIO20 | Native USB connector |

Firmware constants should remain aligned with the PRD:

```c
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_BCK_PIN=4
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_WS_PIN=5
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_DATA_PIN=6
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_PORT=I2S_NUM_0
```

## 3. Electrical Assumptions

The reference build assumes:

- the ESP32-S3 board exposes GPIO4, GPIO5, and GPIO6;
- the ESP32-S3 board routes native USB to GPIO19/GPIO20;
- the USB cable has data lines;
- the INMP441 breakout accepts 3.3 V logic and power;
- microphone I2S wiring is short;
- no external power supply is used during the first MVP bring-up.

If any of these assumptions fail, the build is no longer the v0.1 reference build.

## 4. Accepted Substitutions

Substitutions are allowed for builders, but firmware development should stay on the reference build until v0.1 works.

| Substitute | Status | Notes |
| --- | --- | --- |
| Seeed Studio XIAO ESP32S3 | Good compact alternate | D3/GPIO4, D4/GPIO5, D5/GPIO6 map cleanly to the reference pinout. |
| Waveshare ESP32-S3-Zero | Possible alternate | Verify GPIO4/5/6 availability and native USB behavior. |
| Waveshare ESP32-S3-Tiny N8R8 | Possible alternate | Good compact candidate, but mechanically different from the public default. |
| Generic ESP32-S3 SuperMini | Experimental | Accept only with clear pinout and confirmed real ESP32-S3 chip. |
| Bare ESP32-S3-WROOM module | Future PCB only | Not the public MVP assembly path. |
| PDM microphone | Not v0.1 | Requires firmware and PRD changes. |
| Analog microphone module | Not supported | Requires ADC/codec path and violates the current architecture. |

## 5. Why This Is The Reference

This build gives the firmware a stable target:

- one ESP32-S3 family;
- one microphone family;
- one default pinout;
- one audio rate;
- one USB mode;
- one wiring path.

The main risk at this stage is not that the project lacks flexibility. The risk is debugging firmware against too many slightly different boards. The reference build keeps v0.1 narrow enough to finish.

## 6. v0.1 Firmware Scope Boundary

The first firmware implementation should support:

- ESP32-S3 target through ESP-IDF;
- the fixed I2S pinout above;
- INMP441-style 24-bit I2S capture in 32-bit frames;
- USB Audio Class 1.0 microphone output;
- Wi-Fi AP configuration portal;
- bypass, noise, and pitch controls.

The first firmware implementation should not spend time on:

- board auto-detection;
- alternate pin profiles in the UI;
- PDM microphone support;
- analog microphone support;
- battery support;
- custom PCB pin migration;
- browser-based web installer.

The web installer is a release/distribution feature. The v0.1 development path is ESP-IDF first, compiled binaries later.

## 7. Hardware Work Deferred To v0.2

The following are intentionally deferred until after the v0.1 MVP works:

- KiCad schematic;
- complete PCB layout;
- PCB manufacturing outputs;
- final enclosure design;
- complete production-grade diagrams;
- alternate microphone footprints;
- custom USB-C implementation on a PCB.

For v0.1, the existing conceptual schematic and wiring diagrams are enough.

## 8. Acceptance Criteria

The reference build is accepted when:

- ESP32-S3 board flashes and boots reliably.
- INMP441 is wired on GPIO4/5/6.
- USB native port connects to the host.
- Firmware can see microphone samples.
- Firmware can enumerate as a USB microphone.
- Audio reaches the host in bypass mode.
- Noise and pitch controls audibly affect the signal.
- The build runs for at least 10 minutes without crash, watchdog, or USB disconnect.

## 9. Related Docs

- Public BoM: `docs/hardware/HARDWARE_BOM_PUBLIC.md`
- Schematic concept: `docs/hardware/SCHEMATIC_CONCEPT.md`
- Bring-up checklist: `docs/hardware/BRINGUP_CHECKLIST.md`
- MVP plan: `docs/project/PLAN_V0.1_MVP.md`
