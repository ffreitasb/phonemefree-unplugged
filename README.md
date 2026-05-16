<div align="center">
  <img src="assets/branding/phonemefree-unplugged_icon.svg" alt="PhonemeFree Unplugged icon" width="108">
  <br><br>
  <strong>Speak freely. Leave less voiceprint. No phone required.</strong>
  <br>
  <sub>Air-gapped ESP32-S3 USB voice obfuscation for defensive voice-cloning resistance research.</sub>
</div>

<br>

# PhonemeFree Unplugged

<div align="center">

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-ESP32--S3-e7352c?logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
[![USB Audio](https://img.shields.io/badge/USB%20Audio-UAC%201.0-00599c)](#architecture)
[![Audio](https://img.shields.io/badge/audio-16kHz%20mono%2016--bit-00aa88)](#hardware-target)
[![Offline](https://img.shields.io/badge/network-air--gapped-111111)](#security-and-privacy)
[![Plan](https://img.shields.io/badge/status-v0.1%20planning-blue)](docs/project/PLAN_V0.1_MVP.md)

</div>

PhonemeFree Unplugged is the hardware branch of [PhonemeFree](https://github.com/ffreitasb/PhonemeFree): a small ESP32-S3 firmware project that captures microphone audio, alters the PCM stream locally, and presents the result to a host as a class-compliant USB microphone.

It is not a cloud filter, not a voice deepfake toy, and not an Android app. It is an air-gapped peripheral for studying a narrow defensive question: what happens if the microphone path stops handing clean biometric voiceprints to everything upstream?

## Why This Exists

Voice-cloning systems do not need magic. They need samples.

Most defenses start after audio has already become data: recorded, uploaded, indexed, embedded, or used for model training. PhonemeFree Unplugged moves the defensive boundary into the capture device itself.

The first firmware MVP is deliberately plain:

- Capture voice from an I2S microphone.
- Process it locally on the ESP32-S3.
- Return the altered signal as a USB microphone.
- Keep configuration offline through a local Wi-Fi AP.
- Measure latency, underruns, and stability from there.

## What It Is

PhonemeFree Unplugged is:

- An ESP32-S3 firmware project.
- A USB Audio Class 1.0 microphone device.
- A local `I2S -> DSP -> USB` audio pipeline.
- A small testbed for pitch, noise, and formant-style voice obfuscation.
- A companion hardware path for the main PhonemeFree research project.

## What It Is Not

PhonemeFree Unplugged is not:

- A full anonymity guarantee.
- A network voice encryption tool.
- A hidden recorder.
- A cloud speech service.
- A commercial SDK, SaaS component, or polished consumer product.

## Current Status

Current phase: v0.1 MVP planning and repository setup.

Target release: `v0.1.0`.

Planned for the first functional pass:

- Reference hardware build locked to ESP32-S3-WROOM-1 N8R8/N16R8 + INMP441.
- ESP-IDF scaffold for ESP32-S3.
- I2S capture from INMP441-compatible microphone hardware.
- PCM conversion to 16 kHz / 16-bit / mono.
- DSP pipeline with bypass, pitch shift, and deterministic noise injection.
- USB Audio Class 1.0 output using TinyUSB.
- Offline Wi-Fi AP with captive portal.
- LittleFS-hosted terminal-style control UI.
- WebSocket parameter updates with atomic DSP state.
- Basic Unity tests for components that can run without hardware.

The granular MVP plan lives in `docs/project/PLAN_V0.1_MVP.md`.

## Hardware Target

| Component | Target |
| --- | --- |
| MCU | Reference: ESP32-S3-WROOM-1 N8R8/N16R8 dev board |
| Microphone | Reference: INMP441 I2S microphone module |
| USB | Native ESP32-S3 USB OTG in device mode |
| Audio | 16 kHz, 16-bit, mono PCM |
| Storage | Flash with LittleFS partition for web assets |
| Power | 5V from USB-C host |

Default I2S pins:

| Signal | GPIO |
| --- | --- |
| BCK | 4 |
| WS | 5 |
| DATA | 6 |

## Architecture

```text
INMP441
   |
   v
I2S DMA -> input ring buffer -> DSP engine -> output ring buffer -> TinyUSB UAC
                                      ^
                                      |
                         Wi-Fi AP + captive portal + WebSocket
```

Core firmware map:

| Layer | Planned Location | Notes |
| --- | --- | --- |
| Application init | `main/` | Boot orchestration and task startup |
| I2S input | `components/hal_i2s/` | Microphone capture and PCM conversion |
| Ring buffers | `components/hal_ringbuf/` | Shared ESP-IDF ring buffer instances |
| DSP | `components/dsp_*` | Pitch, noise, optional formant, engine task |
| USB audio | `components/usb_audio_uac/` | TinyUSB UAC 1.0 microphone |
| Wi-Fi AP | `components/wifi_ap/` | Offline access point and DNS captive portal |
| Portal | `components/webserver_portal/` | `esp_http_server`, WebSocket, status API |
| UI assets | `data/` | LittleFS-hosted `index.html` |

## Repository Layout

| Directory | Purpose |
| --- | --- |
| [docs/](docs/) | Public documentation grouped by subject. |
| [docs/project/](docs/project/) | Planning, roadmap, and release scope. |
| [docs/hardware/](docs/hardware/) | Public hardware BoMs, sourcing notes, and assembly references. |
| [docs/firmware/](docs/firmware/) | Firmware architecture and implementation notes. |
| [hardware/](hardware/) | Hardware build material, from breadboard through future PCB work. |
| [hardware/schematic/](hardware/schematic/) | Conceptual schematic diagrams and future schematic sources. |
| [hardware/breadboard/](hardware/breadboard/) | Temporary first-pass wiring and bring-up notes. |
| [hardware/perfboard/](hardware/perfboard/) | DIY double-sided perfboard build notes. |
| [hardware/pcb/](hardware/pcb/) | Future PCB source files and schematic/layout material. |
| [hardware/mechanical/](hardware/mechanical/) | Enclosure, acoustic port, and mounting notes. |
| [hardware/manufacturing/](hardware/manufacturing/) | Future Gerbers, drill files, BoM exports, and release fabrication packages. |
| [assets/branding/](assets/branding/) | Public branding assets used by the repository. |
| [tools/](tools/) | Helper scripts and local automation. |

## Quickstart

This repository is not buildable yet. The first implementation step is the ESP-IDF scaffold described in `docs/project/PLAN_V0.1_MVP.md`.

Expected toolchain:

- ESP-IDF with ESP32-S3 support.
- CMake and Ninja as provided by ESP-IDF.
- Python environment managed by ESP-IDF.
- PowerShell for local helper scripts on Windows.

Expected build flow once the scaffold exists:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## Roadmap

- [x] PRD captured locally.
- [x] v0.1 MVP plan.
- [x] v0.1 reference hardware build.
- [x] Hardware bring-up checklist.
- [ ] ESP-IDF scaffold.
- [ ] Ring buffer component.
- [ ] Deterministic noise DSP.
- [ ] USB microphone enumeration.
- [ ] I2S microphone capture.
- [ ] End-to-end `I2S -> DSP -> USB` audio.
- [ ] Wi-Fi AP and captive portal.
- [ ] WebSocket DSP controls.
- [ ] First hardware acceptance pass.

## Documentation Map

| File | Purpose |
| --- | --- |
| [docs/hardware/HARDWARE_BOM_PUBLIC.md](docs/hardware/HARDWARE_BOM_PUBLIC.md) | Public hardware BoM for AliExpress-style sourcing and DIY assembly. |
| [docs/hardware/REFERENCE_BUILD_V0.1.md](docs/hardware/REFERENCE_BUILD_V0.1.md) | Locked reference hardware build for the first firmware MVP. |
| [docs/hardware/BRINGUP_CHECKLIST.md](docs/hardware/BRINGUP_CHECKLIST.md) | Bench checklist for validating hardware before firmware debugging. |
| [docs/hardware/SCHEMATIC_CONCEPT.md](docs/hardware/SCHEMATIC_CONCEPT.md) | Conceptual schematic, wiring diagrams, and PCB block direction. |
| [docs/project/PLAN_V0.1_MVP.md](docs/project/PLAN_V0.1_MVP.md) | Granular plan from repository setup to first firmware MVP. |
| [docs/README.md](docs/README.md) | Documentation directory map. |
| [hardware/README.md](hardware/README.md) | Hardware directory map. |
| [assets/README.md](assets/README.md) | Assets directory map. |
| [tools/README.md](tools/README.md) | Tooling directory map. |
| [README.md](README.md) | Project overview, status, and development map. |

More project documents will be added as the firmware takes shape.

## Security And Privacy

The intended posture is local-first and air-gapped:

- No cloud audio processing.
- No internet routing from the device AP.
- No persistent audio logging.
- No portal authentication in the first MVP.
- No hidden recording path.

PhonemeFree Unplugged does not claim universal biometric anonymity. It is a defensive research tool whose protection level must be measured against real voices, real rooms, real microphones, and real models.

## Name

`Phoneme` is the minimum distinctive unit of speech sound. It is also one of the layers modern voice systems learn to bind to speaker identity.

`Free` is a deliberate reference to SpeakFreely, an early encrypted VoIP tool from an older internet where privacy was often an engineering act rather than a product checkbox.

`Unplugged` marks the hardware path: local signal processing, USB output, offline configuration, and no dependency on phone audio routing.

PhonemeFree Unplugged is the inverse of a voice deepfake: instead of synthesizing a voice identity, it tries to damage the capture path that preserves one.

---

Built for people who would rather not donate their voiceprint to the next model by accident.
