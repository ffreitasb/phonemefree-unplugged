# AGENTS.md

Project instructions for Codex sessions working on PhonemeFree Unplugged.

## Project Snapshot

- PhonemeFree Unplugged is an ESP32-S3 firmware/hardware project for local voice obfuscation.
- The device captures microphone audio, processes PCM locally, and exposes the result to a host as a USB microphone.
- The privacy posture is local-first and air-gapped: no cloud audio, no internet routing, no hidden recording path.
- User-facing conversation in this repository is usually in Brazilian Portuguese. Repository documentation is currently mostly English and ASCII.

## Source Of Truth

- If `PRD_PhonemeFree-unplugged.md` exists locally, read it before making product or architecture decisions. It is intentionally ignored by Git.
- Treat `docs/project/PLAN_V0.1_MVP.md` as the public implementation plan for v0.1.
- Treat `docs/hardware/REFERENCE_BUILD_V0.1.md` as the hardware target for firmware work.
- Do not change product scope, reference hardware, audio format, or privacy posture without making the decision explicit in docs.

## Current v0.1 Target

- MCU target: ESP32-S3.
- Reference board: ESP32-S3-WROOM-1 N8R8/N16R8 USB-C development board.
- Reference microphone: INMP441-compatible I2S MEMS microphone module.
- Native USB pins are reserved for USB: GPIO19 = D-, GPIO20 = D+.
- Default I2S pins:
  - BCLK: GPIO4.
  - WS/LRCLK: GPIO5.
  - DATA/DOUT: GPIO6.
- Audio format:
  - USB/DSP PCM: 16 kHz, 16-bit, mono.
  - I2S input: 24-bit microphone data in 32-bit frames.
  - Input ring buffer target: 512 samples.
  - DSP output ring buffer target: 64 samples.
- USB identity:
  - VID: `0x303A`.
  - PID: `0x4001`.
  - Product: `PhonemeFree Unplugged Mic`.
- Wi-Fi AP SSID: `PhonemeFree Unplugged`.

## Firmware Architecture

Use ESP-IDF first. Do not migrate to Arduino unless explicitly requested.

Expected component map:

- `main/`: app boot orchestration.
- `components/hal_i2s/`: I2S microphone capture and 32-bit to 16-bit PCM conversion.
- `components/hal_ringbuf/`: shared ESP-IDF ring buffers.
- `components/dsp_noise/`: deterministic noise obfuscation.
- `components/dsp_pitch/`: pitch shifting path.
- `components/dsp_formant/`: optional formant path, disabled by default in v0.1.
- `components/dsp_engine/`: DSP task and `_Atomic` parameter state.
- `components/usb_audio_uac/`: TinyUSB/ESP TinyUSB UAC microphone output.
- `components/wifi_ap/`: offline SoftAP.
- `components/webserver_portal/`: `esp_http_server`, WebSocket, and status endpoints.
- `data/`: LittleFS web UI assets.

## Hard Constraints

- Use ESP-IDF with target `esp32s3`.
- Use `espressif/esp_tinyusb` for ESP-IDF TinyUSB integration.
- Use LittleFS for web assets, not SPIFFS.
- Use `esp_http_server`, not AsyncWebServer.
- Do not use cJSON in the firmware path; keep portal JSON small and hand-written unless a documented parser is approved.
- DSP parameters shared across tasks must use `_Atomic`; do not use `volatile` as a synchronization substitute.
- Do not allocate heap in real-time audio hot paths.
- USB underrun should emit silence instead of blocking.
- I2S/DSP overflow should fail or drop without blocking the hot path and should increment diagnostics.
- Keep Wi-Fi and webserver work away from the audio hot path.

## Hardware Scope Decisions

- v0.1 is I2S microphone first.
- A 3.5 mm P2/TRS/TRRS microphone input is only a low-priority future exploration. If added later, prefer an external analog front-end or audio codec that outputs I2S; avoid making raw ESP32-S3 ADC audio the main path.
- Bluetooth headset or Bluetooth microphone connectivity is discarded by design and ESP32-S3 hardware limitations. It is not in the implementation pipeline.
- KiCad, custom PCB, production schematics, and manufacturing files are v0.2+ work, not v0.1 blockers.

## Local Toolchain

Validated Windows setup:

```powershell
eim --do-not-track true run "idf.py --version"
eim --do-not-track true run "idf.py set-target esp32s3"
eim --do-not-track true run "idf.py build"
```

Known installed local version: ESP-IDF `v6.0.1`.

Generated local files that should stay untracked:

- `build/`
- `sdkconfig`
- `sdkconfig.old`
- `managed_components/`
- `eim_config.toml`

`dependencies.lock` should remain trackable because it pins Component Manager versions.

## Repository Hygiene

- Keep edits scoped to the current task.
- Do not commit or push unless the user asks for it.
- Preserve ignored/internal files:
  - `PRD_PhonemeFree-unplugged.md`
  - `ideas/`
  - `HARDWARE_BOM_IDEAL.md`
  - `assets/internal/`
  - `docs/internal/`
  - `internal/`
- Public hardware docs live under `docs/hardware/`; private notes stay ignored.
- Prefer adding short, useful docs when changing behavior or scope.
- Keep public docs readable for DIY builders; do not expose internal planning notes unless requested.

## Next Implementation Bias

Current firmware core builds with I2S capture, DSP task, and an initial TinyUSB UAC2 microphone feeder. Next, prioritize:

1. Flash and serial monitor on the reference ESP32-S3 board.
2. Host validation of USB microphone enumeration and UAC2 compatibility.
3. Hardware bring-up of INMP441 I2S capture on GPIO4/5/6.
4. End-to-end `I2S -> DSP -> USB` audio validation.
5. Wi-Fi AP, captive portal, and portal controls writing `g_dsp_params` with `atomic_store`.
