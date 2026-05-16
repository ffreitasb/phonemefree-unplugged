# Firmware Core Test App

ESP-IDF Unity test app for hardware-independent firmware components.

Covered components:

- `hal_ringbuf`
- `dsp_noise`
- `dsp_engine` parameter/state reset

Build from the repository root:

```powershell
Push-Location test_apps/firmware_core
eim --do-not-track true run "idf.py set-target esp32s3 build"
Pop-Location
```

This app intentionally does not test I2S capture or USB enumeration. Those require hardware bring-up.
