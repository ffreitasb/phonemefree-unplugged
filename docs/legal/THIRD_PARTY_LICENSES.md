# Third-Party Licenses

PhonemeFree Unplugged depends on third-party software and tools. Those dependencies keep their original licenses. This project does not relicense them under the project licenses.

## Current Firmware Dependencies

The current ESP-IDF Component Manager dependencies are pinned in [../../dependencies.lock](../../dependencies.lock).

| Dependency | Current pinned version | Upstream license family | Notes |
| --- | ---: | --- | --- |
| ESP-IDF | `6.0.1` | Apache-2.0 plus bundled third-party notices | ESP-IDF includes additional third-party components under their own notices. |
| `espressif/esp_tinyusb` | `2.2.0` | Apache-2.0 | ESP-IDF TinyUSB integration component. |
| `espressif/tinyusb` | `0.19.0~3` | MIT | TinyUSB device stack dependency pulled by `esp_tinyusb`. |
| `espressif/esp-dsp` | `1.8.2` | Apache-2.0 | DSP helper library from Espressif. |
| `joltwallet/littlefs` | `1.21.1` | MIT | LittleFS integration component. |

The source tree generated under `managed_components/` is intentionally untracked. When preparing a firmware binary release, regenerate or archive those components and include their upstream `LICENSE`, `NOTICE`, and copyright files where applicable.

## Release Checklist

Before publishing binaries, verify that the release artifact or release page includes:

- A copy of this project's `LICENSE.md`.
- A copy of `LICENSES/AGPL-3.0-or-later.txt`.
- A copy of `LICENSES/CERN-OHL-S-2.0.txt` if hardware files are included.
- Third-party license files for bundled or statically linked managed components.
- The exact `dependencies.lock` used for the build.
- The ESP-IDF version used for the build.
- Build and installation instructions.

## Notes

Apache-2.0, MIT, BSD-style, and similar permissive dependencies can be used in an `AGPL-3.0-or-later` firmware project, but their notices still need to travel with redistributions where required.

If a future dependency uses GPL, LGPL, MPL, proprietary, non-commercial, source-available, or unclear terms, review it before adding it to firmware or release artifacts.
