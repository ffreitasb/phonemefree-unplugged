# ESP-IDF Setup

This project uses ESP-IDF as the native firmware development environment.

Current local setup:

| Item | Value |
| --- | --- |
| Installer | Espressif Installation Manager CLI (`eim`) |
| ESP-IDF | `v6.0.1` |
| IDF path | `C:\esp\v6.0.1\esp-idf` |
| Tools path | `C:\Espressif\tools` |
| Target | `esp32s3` |
| Validated build | `build\phonemefree-unplugged.bin` |

## Windows Commands

The current Windows installation was created with:

```powershell
winget install Espressif.EIM-CLI --accept-package-agreements --accept-source-agreements
eim --do-not-track true install
```

Use EIM to run ESP-IDF commands from a regular PowerShell session:

```powershell
eim --do-not-track true run "idf.py --version"
eim --do-not-track true run "idf.py set-target esp32s3"
eim --do-not-track true run "idf.py build"
```

The EIM installer also creates an ESP-IDF PowerShell shortcut with the environment already activated.

To flash after connecting an ESP32-S3 board:

```powershell
eim --do-not-track true run "idf.py -p COMx flash monitor"
```

Replace `COMx` with the board serial port.

## Component Manager

The scaffold currently pins component versions through `dependencies.lock` after the first successful solve.

Direct firmware dependencies:

- `espressif/esp_tinyusb` for native ESP-IDF TinyUSB integration.
- `espressif/esp-dsp` for future DSP helpers.
- `joltwallet/littlefs` for the offline local web UI partition.

## Notes

- `idf.py` is intentionally not committed into the repository.
- `build/`, `sdkconfig`, `sdkconfig.old`, `managed_components/`, and `eim_config.toml` are local generated files.
- `dependencies.lock` should remain trackable once generated, because it pins component versions.
- Release binaries and browser-based flashing are deferred until release packaging.
