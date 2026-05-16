# Firmware Docs

Firmware is developed with ESP-IDF for the ESP32-S3 reference build.

Current v0.1 path:

- `hal_i2s` configures I2S STD RX at 16 kHz, 32-bit frames, left slot, GPIO4/5/6.
- `hal_i2s` converts INMP441-style 24-bit-left-aligned frames to 16-bit PCM.
- `dsp_engine` consumes the input ring buffer and writes processed PCM to the DSP output ring buffer.
- `dsp_noise` is deterministic and test-covered.
- `usb_audio_uac` installs TinyUSB as a UAC2 mono microphone, exposes the project VID/PID/product strings, and feeds 16 kHz / 16-bit PCM from the DSP output ring buffer.
- USB underruns are filled with silence and counted for diagnostics.
- Wi-Fi AP, captive portal, LittleFS serving, WebSocket controls, and real pitch shifting are still scaffold or future v0.1 work.

Technical note: the current TinyUSB path uses the bundled UAC2 microphone descriptor helper. The PRD mentions UAC 1.0; host compatibility must be validated on hardware before deciding whether to keep UAC2 for v0.1 or replace the descriptor with a manual UAC1 implementation.

Build commands:

```powershell
eim --do-not-track true run "idf.py build"
Push-Location test_apps/firmware_core
eim --do-not-track true run "idf.py build"
Pop-Location
```

| File | Purpose |
| --- | --- |
| [ESP_IDF_SETUP.md](ESP_IDF_SETUP.md) | Native ESP-IDF installation and command notes. |
| [firmware_core test app](../../test_apps/firmware_core/README.md) | ESP-IDF Unity test app for firmware components that do not need hardware. |
