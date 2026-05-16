# License

PhonemeFree Unplugged uses a dual-license model.

This repository contains both open hardware design material and firmware/software. Those are different kinds of work, so they are licensed separately.

## Summary

| Area | License | SPDX identifier |
| --- | --- | --- |
| Hardware design source, PCB files, schematic files, enclosure/mechanical files, manufacturing exports, and hardware build documentation | CERN Open Hardware Licence Version 2 - Strongly Reciprocal | `CERN-OHL-S-2.0` |
| Firmware source code, embedded web UI, build files, tests, tooling, and firmware/project documentation | GNU Affero General Public License v3.0 or later | `AGPL-3.0-or-later` |

The full license texts are included in:

- [LICENSES/CERN-OHL-S-2.0.txt](LICENSES/CERN-OHL-S-2.0.txt)
- [LICENSES/AGPL-3.0-or-later.txt](LICENSES/AGPL-3.0-or-later.txt)

If a file contains an explicit `SPDX-License-Identifier`, that file-level notice controls.

## Hardware

Hardware source is licensed under `CERN-OHL-S-2.0`.

This includes, when present:

- Schematics and schematic source files.
- PCB layout source files.
- Gerbers, drill files, pick-and-place exports, and fabrication packages.
- Enclosure, mechanical, CAD, and mounting files.
- Hardware assembly, bring-up, and manufacturing documentation.
- Hardware diagrams and wiring references.

The intent is simple: people can study, make, modify, and distribute the hardware, but distributed derivatives must preserve the same open-hardware reciprocity.

## Firmware And Software

Firmware and software are licensed under `AGPL-3.0-or-later`.

This includes:

- ESP-IDF firmware code.
- DSP, I2S, USB, Wi-Fi, and portal components.
- The embedded web UI served from device flash.
- Build files and test applications.
- Tooling and scripts authored for this project.
- Firmware and project documentation unless a narrower file-level notice says otherwise.

The intent is also simple: modified firmware that is distributed, or modified network-interactive firmware that is made available to users, must keep the corresponding source available under the same license family.

## Third-Party Code

Third-party dependencies keep their upstream licenses. The project license does not relicense ESP-IDF, managed ESP-IDF components, toolchains, or vendor libraries.

See [docs/legal/THIRD_PARTY_LICENSES.md](docs/legal/THIRD_PARTY_LICENSES.md) for the current dependency notice policy.

## Branding

The project licenses do not grant trademark rights.

The `PhonemeFree` and `PhonemeFree Unplugged` names, logos, icons, and visual identity may be used to identify this project and unmodified redistributions, but they may not be used to imply endorsement, official status, or compatibility for modified products without permission.

See [docs/legal/BRANDING_POLICY.md](docs/legal/BRANDING_POLICY.md).

## Practical Guidance

When adding new files:

- Use `SPDX-License-Identifier: AGPL-3.0-or-later` for firmware, software, tests, tools, and embedded UI assets.
- Use `SPDX-License-Identifier: CERN-OHL-S-2.0` for hardware design source and manufacturing files.
- Keep third-party files under their original notices.
- Do not remove upstream license notices from dependencies.

When publishing a binary firmware release, include enough corresponding source, build instructions, dependency version information, and installation information for users to rebuild and install a modified version on the same class of device.

This file is a project policy summary, not legal advice.
