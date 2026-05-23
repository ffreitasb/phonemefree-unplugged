# AGENTS.md

Project instructions for Codex sessions working on PhonemeFree Unplugged.

## Project Snapshot

- PhonemeFree Unplugged is an ESP32-S3 firmware/hardware project for local voice obfuscation.
- The device captures microphone audio, processes PCM locally, and exposes the result to a host as a USB microphone.
- The privacy posture is local-first and radio-silent by default: no cloud audio, Wi-Fi off during normal USB microphone operation, no internet routing, no hidden recording path.
- User-facing conversation in this repository is usually in Brazilian Portuguese. Repository documentation is currently mostly English and ASCII.

## Source Of Truth

- If `PRD_PhonemeFree-unplugged.md` exists locally, read it before making product or architecture decisions. It is intentionally ignored by Git.
- Treat `docs/project/PLAN_V0.1_MVP.md` as the public implementation plan for v0.1.
- Treat `docs/hardware/REFERENCE_BUILD_V0.1.md` as the hardware target for firmware work.
- Do not change product scope, reference hardware, audio format, or privacy posture without making the decision explicit in docs.

## Current v0.1 Target

- MCU target: ESP32-S3.
- Reference board: ESP32-S3-WROOM-1 N8R8/N16R8 USB-C development board.
- Reference microphone: ICS-43434 I2S MEMS microphone breakout.
- Fallback microphone: INMP441-compatible I2S MEMS microphone module.
- Budget fallback under investigation: confirm whether the intended part is MS3625, MSM261S4030H0, or another clone/module.
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
- Wi-Fi radio policy: off by default; configuration AP is only started during a physical maintenance window.
- Wi-Fi AP SSID: `PhonemeFree Unplugged`.
- Wi-Fi AP security: WPA2 required; no open AP for v0.1 acceptance.
- Wi-Fi AP timeout policy: stop after 2 minutes without client/heartbeat, and always stop after a 10 minute hard cap.
- Wi-Fi AP trigger: physical button from safe spare GPIO to GND with pull-up; ISR must only notify a task/event queue; never call `esp_wifi_start()` from the ISR.

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
- `components/wifi_ap/`: radio-silent-by-default WPA2 SoftAP lifecycle.
- `components/config_button/` or equivalent: physical maintenance trigger task/event path.
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
- Do not start Wi-Fi or the HTTP server during normal boot after the on-demand AP refactor lands.
- Do not call Wi-Fi, HTTP server, NVS, or logging-heavy code from a GPIO ISR; ISRs only notify a task/timer/queue.

## Hardware Scope Decisions

- v0.1 is I2S microphone first.
- ICS-43434 is the official/preferred v0.1 microphone target.
- INMP441 is a common fallback and should be documented with measured caveats rather than treated as the best target.
- A 3.5 mm P2/TRS/TRRS microphone input is only a low-priority future exploration. If added later, prefer an external analog front-end or audio codec that outputs I2S; avoid making raw ESP32-S3 ADC audio the main path.
- Bluetooth headset or Bluetooth microphone connectivity is discarded by design and ESP32-S3 hardware limitations. It is not in the implementation pipeline.
- KiCad, custom PCB, production schematics, and manufacturing files are v0.2+ work, not v0.1 blockers.

## Licensing Model

- Treat `LICENSE.md` and `docs/legal/` as the licensing source of truth.
- Hardware source, hardware diagrams, PCB/schematic/mechanical/manufacturing files, and public hardware build documentation use `CERN-OHL-S-2.0`.
- Firmware, embedded web UI, tests, tools, build files, firmware docs, and project docs use `AGPL-3.0-or-later`.
- Third-party files keep their upstream notices; do not replace dependency license headers with project headers.
- New project-authored firmware/software files should carry `SPDX-License-Identifier: AGPL-3.0-or-later`.
- New project-authored hardware source files should carry `SPDX-License-Identifier: CERN-OHL-S-2.0`.
- Project branding is reserved. The open licenses do not grant trademark rights for the `PhonemeFree` or `PhonemeFree Unplugged` names/logos.

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

Current firmware builds with I2S capture, DSP task, initial TinyUSB UAC2 microphone feeder, open Wi-Fi SoftAP scaffold, LittleFS UI image generation, `/api/status`, and a minimal `/ws` control path. The open AP is development-only and must be replaced before v0.1 acceptance. Next, prioritize:

1. Flash and serial monitor on the reference ESP32-S3 board.
2. Host validation of USB microphone enumeration and UAC2 compatibility.
3. Hardware bring-up of ICS-43434 I2S capture on GPIO4/5/6, with INMP441 fallback comparison when available.
4. End-to-end `I2S -> DSP -> USB` audio validation.
5. Refactor Wi-Fi to radio-silent-by-default WPA2 AP with physical button, 2 minute inactivity timeout, and 10 minute hard cap.
6. Browser validation of AP portal, `/api/status`, and `/ws` controls inside the temporary maintenance window.
7. Captive DNS redirect and real pitch shifting.
