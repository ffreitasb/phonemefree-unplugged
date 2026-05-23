# Bring-Up Checklist - PhonemeFree Unplugged v0.1

This checklist is the bench procedure for the v0.1 reference build.

Use it before firmware debugging. If a step fails, fix the hardware or setup first.

Reference build:

```text
ESP32-S3-WROOM-1 N8R8/N16R8 USB-C development board
+ ICS-43434 I2S microphone breakout
+ INMP441 I2S microphone fallback, if available
+ tactile configuration button on safe spare GPIO, exact GPIO TBD
+ GPIO4 BCLK
+ GPIO5 WS
+ GPIO6 DATA
+ native USB for host audio
```

## 1. Parts Check

- [ ] ESP32-S3 board is a real ESP32-S3, not ESP32-C3/C6/S2.
- [ ] Board exposes GPIO4, GPIO5, and GPIO6.
- [ ] Board exposes at least one extra GPIO for the physical configuration button.
- [ ] Board has access to native USB for device mode.
- [ ] Board has BOOT and RESET buttons.
- [ ] Board has a 3V3 pin.
- [ ] Microphone module is I2S, not analog and not PDM-only.
- [ ] Microphone module exposes `SCK/BCLK`, `WS/LRCLK`, `SD/DOUT`, `VDD`, `GND`, and preferably `SEL/L/R`.
- [ ] Tactile configuration button is available for the temporary AP trigger.
- [ ] USB cable is a data cable.
- [ ] Host computer can run ESP-IDF tooling for development.

## 2. Visual Inspection

- [ ] No bent pins on the ESP32-S3 board.
- [ ] No solder bridges on microphone header.
- [ ] Microphone acoustic port is not blocked.
- [ ] Microphone module orientation is understood.
- [ ] USB connector is mechanically stable.
- [ ] Breadboard/perfboard has no loose metal scraps.

Take a photo of the wiring before first power. It saves time later.

## 3. Wiring

With power disconnected:

| ESP32-S3 Dev Board | ICS/INMP441-style Module | Check |
| --- | --- | --- |
| `3V3` | `VDD` / `3V` | [ ] |
| `GND` | `GND` | [ ] |
| `GPIO4` | `SCK` / `BCLK` | [ ] |
| `GPIO5` | `WS` / `LRCLK` | [ ] |
| `GPIO6` | `SD` / `DOUT` | [ ] |
| `GND` | `SEL` / `L/R` | [ ] |

Rules:

- [ ] I2S wires are short.
- [ ] GPIO19 and GPIO20 are not connected to the microphone.
- [ ] Configuration button is wired from the selected spare GPIO to GND, expecting firmware/internal pull-up.
- [ ] Configuration button is not connected to GPIO19/GPIO20, GPIO4/GPIO5/GPIO6, or a risky boot strap pin.
- [ ] No signal wire is connected to 5 V.
- [ ] `VDD` on the microphone is connected to 3.3 V, not 5 V.

## 4. Power-On Sanity Check

Before flashing project firmware:

- [ ] Connect ESP32-S3 to USB.
- [ ] Power LED turns on.
- [ ] Board does not heat up.
- [ ] Microphone does not heat up.
- [ ] Host detects a USB device or serial/JTAG interface.
- [ ] Pressing RESET restarts the board.
- [ ] BOOT mode can be entered if needed.

If available:

- [ ] Measure 3.3 V rail.
- [ ] Measure continuity between ESP32-S3 GND and microphone GND.
- [ ] Confirm microphone VDD is near 3.3 V.

## 5. ESP-IDF Environment

This project uses ESP-IDF for development.

- [ ] ESP-IDF is installed.
- [ ] `idf.py --version` works.
- [ ] `idf.py set-target esp32s3` works once the scaffold exists.
- [ ] Host has permission to access the ESP32-S3 serial/USB device.

Expected development flow after scaffold:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

Browser-based installation is deferred to release packaging. It is not part of early firmware development.

## 6. Scaffold Smoke Test

After the ESP-IDF scaffold exists:

- [ ] `idf.py build` completes.
- [ ] `idf.py flash monitor` completes.
- [ ] Boot log prints project name/version.
- [ ] Target is `esp32s3`.
- [ ] No boot loop.
- [ ] No watchdog.
- [ ] No panic.

## 7. I2S Bring-Up

When `hal_i2s` exists:

- [ ] Firmware config uses GPIO4 for BCLK.
- [ ] Firmware config uses GPIO5 for WS.
- [ ] Firmware config uses GPIO6 for DATA.
- [ ] I2S driver initializes.
- [ ] DMA starts.
- [ ] Ring buffer receives bytes.
- [ ] Silence produces low-amplitude samples.
- [ ] Finger tap or voice near mic changes sample amplitude.
- [ ] If samples are always zero, test `SEL` / `L/R` tied to 3V3.
- [ ] If samples are noise only, shorten wiring and re-check ground.

Optional if a logic analyzer is available:

- [ ] BCLK is present.
- [ ] WS/LRCLK is present.
- [ ] DATA changes when sound is present.

## 8. DSP Bring-Up

When `dsp_engine` exists:

- [ ] Bypass path copies audio from input ring buffer to output ring buffer.
- [ ] `enabled=false` bypass works.
- [ ] `noise=0` leaves signal unchanged except normal pipeline conversion.
- [ ] `noise>0` audibly changes signal.
- [ ] `pitch=0` takes the fast/no-change path.
- [ ] `pitch!=0` audibly changes signal.
- [ ] No heap allocation occurs inside `IRAM_ATTR` hot path functions.

## 9. USB Audio Bring-Up

When `usb_audio_uac` exists:

- [ ] Device enumerates as `PhonemeFree Unplugged Mic`.
- [ ] Host sees a mono input device.
- [ ] Audio format is 16 kHz / 16-bit / mono.
- [ ] USB callback sends exactly 16 samples per 1 ms frame.
- [ ] On underrun, firmware sends silence rather than blocking.
- [ ] Host does not disconnect during idle silence.
- [ ] Host receives bypass audio from microphone.

Suggested host checks:

- [ ] Windows sound input meter moves.
- [ ] Linux `arecord -l` or equivalent sees the device.
- [ ] A short recording can be captured and played back.

## 10. Wi-Fi Portal Bring-Up

When Wi-Fi and portal exist:

- [ ] AP `PhonemeFree Unplugged` does not appear after normal boot.
- [ ] Physical maintenance button opens the temporary AP window.
- [ ] AP `PhonemeFree Unplugged` appears only during the maintenance window.
- [ ] Client connects via WPA2.
- [ ] Device remains USB-enumerated while Wi-Fi is active.
- [ ] `http://192.168.4.1/` serves the UI.
- [ ] `/api/status` returns JSON.
- [ ] `/ws` connects.
- [ ] Slider changes update atomic DSP parameters.
- [ ] AP stops after 2 minutes without client/heartbeat.
- [ ] AP stops after 10 minutes even with a client connected.
- [ ] Bad JSON does not crash the device.

## 11. Stability Pass

Before calling the reference build usable:

- [ ] Run bypass audio for 10 minutes.
- [ ] Run noise enabled for 10 minutes.
- [ ] Run pitch enabled for 10 minutes.
- [ ] Toggle bypass/enable repeatedly through WebSocket UI.
- [ ] During the maintenance window, disconnect/reconnect Wi-Fi client while USB audio is active.
- [ ] No watchdog.
- [ ] No panic.
- [ ] No USB disconnect.
- [ ] Underrun/overflow counters are visible in log or status endpoint.

## 12. Failure Notes

Use this table during bring-up:

| Symptom | First Things To Check |
| --- | --- |
| Board not detected | USB cable data lines, BOOT mode, correct USB port, driver/permission. |
| Mic samples always zero | `SEL` / `L/R` strap, DATA pin, 3V3, wrong GPIO, channel selection. |
| Mic samples all noise | Ground, long wires, wrong I2S format, loose breadboard connection. |
| USB audio disconnects | Power stability, USB cable, TinyUSB callback blocking, underrun behavior. |
| Wi-Fi breaks audio | Confirm radio is off during normal operation, task affinity, logging volume, priority, Core 1 DSP isolation. |
| Random resets | Brownout, bad USB port, short circuit, insufficient regulator margin. |

## 13. Exit Criteria

Hardware bring-up is complete when:

- [ ] Reference build matches `docs/hardware/REFERENCE_BUILD_V0.1.md`.
- [ ] Firmware scaffold builds and flashes.
- [ ] I2S produces changing microphone samples.
- [ ] USB audio enumerates.
- [ ] Audio reaches host in bypass mode.
- [ ] DSP controls affect the audio.
- [ ] Wi-Fi portal updates parameters only during a WPA2 maintenance window.
- [ ] Wi-Fi radio is off again after the maintenance window closes.
- [ ] 10-minute stability pass succeeds.

After this checklist passes, the project can move toward `v0.1.0` firmware hardening.
