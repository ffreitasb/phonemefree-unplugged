# Release Compliance Checklist

Use this checklist before publishing firmware binaries, hardware manufacturing packages, or public release tags.

## Firmware Binary Release

- [ ] Confirm `git status` is clean.
- [ ] Record the commit SHA used for the build.
- [ ] Record the ESP-IDF version.
- [ ] Include `dependencies.lock`.
- [ ] Include `sdkconfig.defaults`.
- [ ] Include `partitions.csv`.
- [ ] Include source for `main/`, `components/`, `data/`, `tools/`, and relevant `test_apps/`.
- [ ] Include build instructions for the same class of ESP32-S3 target.
- [ ] Include flash/install instructions for modified firmware.
- [ ] Include `LICENSE.md` and `LICENSES/AGPL-3.0-or-later.txt`.
- [ ] Include third-party license notices for managed components included in the build.
- [ ] Verify the device UI or release page tells users where to get corresponding source.

## Hardware Release

- [ ] Include `LICENSE.md` and `LICENSES/CERN-OHL-S-2.0.txt`.
- [ ] Include preferred-form source for schematics, PCB, and mechanical files.
- [ ] Include fabrication outputs derived from those sources, if manufacturing files are published.
- [ ] Include BoM and assembly notes.
- [ ] Include known safety, power, pinout, and bring-up constraints.
- [ ] Mark modified hardware clearly if it differs from upstream.

## Branding

- [ ] Do not label modified firmware or hardware as official.
- [ ] Use a distinct fork/product name for modified releases.
- [ ] Keep attribution factual and visible.

## Notes

This checklist is intentionally practical. It does not replace the license texts.
