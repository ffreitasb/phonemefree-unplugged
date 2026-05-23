# Public Hardware BoM - PhonemeFree Unplugged

This is the public, builder-facing hardware bill of materials for PhonemeFree Unplugged.

It is written for people buying parts from AliExpress or similar hobby electronics catalogs. It avoids lab-only tools and focuses on the parts needed to assemble the first usable device.

The locked v0.1 reference build is documented in `REFERENCE_BUILD_V0.1.md`.

## 1. Short Answer

For the MVP, buy:

| Qty | Part | Recommended Search Term |
| ---: | --- | --- |
| 1 | ESP32-S3 development board with native USB | `ESP32-S3 WROOM N16R8 USB C development board` |
| 1 | I2S MEMS microphone module | `ICS-43434 I2S microphone breakout` |
| 1 optional | Fallback I2S MEMS microphone module | `INMP441 I2S microphone module ESP32` |
| 1 | Physical configuration button | `6x6 tactile push button` |
| 1 | USB data cable | `USB C data cable short` |
| 1 | Breadboard or double-sided perfboard | `double side prototype PCB 2.54mm` |
| 1 set | Dupont wires or solder wire | `dupont jumper wire female female` / `30AWG wire wrap wire` |
| 1 set | Pin headers / sockets | `2.54mm female pin header` |

Recommended public MVP combination:

```text
ESP32-S3-WROOM-1 N8R8/N16R8 USB-C dev board
+ ICS-43434 I2S microphone breakout
+ tactile button for temporary configuration AP
+ short USB data cable
+ double-sided 2.54 mm perfboard once breadboard proof works
```

## 2. MCU Board Requirements

The board does not need to be only a WROOM board, but it must be a real ESP32-S3 board.

Required:

- ESP32-S3 MCU, not ESP32, ESP32-C3, ESP32-C6, ESP32-S2, or RP2040.
- Native USB connected to the ESP32-S3 USB peripheral.
- USB-C or Micro-USB with data lines, not power-only.
- At least 4 MB flash.
- GPIO access for the I2S microphone.
- One spare GPIO for the physical configuration button.
- 3.3 V output pin for the microphone.

Strongly recommended:

- 8 MB flash or more.
- PSRAM, preferably 8 MB.
- Clearly documented pinout.
- Exposed GPIO4, GPIO5, and GPIO6, because these are the default firmware pins.
- Exposed spare GPIO that is not native USB, not I2S, and not a risky boot strap.
- BOOT and RESET buttons.

## 3. ESP32-S3 Board Options

### Option A - Full-size ESP32-S3-WROOM dev board

Best public default.

Search on AliExpress:

- `ESP32-S3 WROOM N16R8 USB C development board`
- `ESP32-S3 WROOM N8R8 USB C development board`
- `ESP32-S3 DevKitC-1 N8R8`
- `ESP32-S3 DevKitC-1 N16R8`

Choose if the listing says:

- `ESP32-S3-WROOM-1`;
- `N8R8`, `N16R8`, or similar flash/PSRAM variant;
- USB-C or native USB;
- GPIO4, GPIO5, GPIO6 exposed.

Why it is preferred:

- More pins exposed.
- Easier wiring.
- Fewer surprises during firmware bring-up.
- Usually closer to Espressif reference designs.

Verdict:

```text
Recommended for first build.
```

### Option B - Seeed Studio XIAO ESP32S3

Good compact option.

Search on AliExpress:

- `Seeed Studio XIAO ESP32S3`
- `XIAO ESP32S3 no camera`
- `XIAO ESP32-S3 8MB PSRAM`

Notes:

- The regular XIAO ESP32S3 is small, USB-C, documented, and friendly for a compact build.
- Its documented pinout exposes:
  - D3 = GPIO4
  - D4 = GPIO5
  - D5 = GPIO6
- That means it can match the default PRD pinout nicely.
- The `Sense` variant includes camera/PDM microphone hardware that is not needed for this project.

Verdict:

```text
Recommended compact board.
```

### Option C - Waveshare ESP32-S3-Zero / ESP32-S3-Tiny

Good compact/hacker option.

Search on AliExpress:

- `Waveshare ESP32-S3-Zero`
- `Waveshare ESP32-S3-Tiny N8R8`
- `ESP32-S3 Tiny USB C N8R8`

Notes:

- These boards are compact and better documented than many generic mini boards.
- Prefer the `N8R8` variant when available.
- Confirm GPIO4, GPIO5, and GPIO6 availability before buying.
- Some tiny boards require more careful soldering and mechanical support.

Verdict:

```text
Good for compact builds after the first full-size build works.
```

### Option D - Generic ESP32-S3 SuperMini

Cheap, compact, but inconsistent.

Search on AliExpress:

- `ESP32-S3 SuperMini`
- `ESP32 S3 Super Mini USB C`
- `ESP32-S3 SuperMini 4MB`

Accept only if:

- The listing clearly says ESP32-S3.
- The pinout shows IO4, IO5, and IO6.
- USB-C is connected for data/programming.
- The board is not actually ESP32-C3.

Avoid if:

- The listing says RISC-V.
- The listing says ESP32-C3.
- The photos and title disagree.
- There is no pinout image.
- Reviews mention wrong chip, boot loops, or fake S3.

Verdict:

```text
Works for experimenters, not recommended as the public default.
```

## 4. Should It Be WROOM Only?

No.

WROOM is the safest family for a first dev board and for a future proper PCB, but compact ESP32-S3 boards can work.

Use this rule:

| Board Type | Public Suitability | Notes |
| --- | --- | --- |
| ESP32-S3-WROOM dev board | Excellent | Best first build, easiest wiring |
| XIAO ESP32S3 | Very good | Compact, documented, default pins available |
| Waveshare ESP32-S3-Zero/Tiny | Good | Compact, check exact variant/pinout |
| ESP32-S3 SuperMini generic | Risky | Cheap, but listings vary |
| Bare ESP32-S3-WROOM module | Not for beginners | Good for PCB, awkward for perfboard |
| ESP32-S3-PICO module/board | Acceptable if documented | Compact, but confirm flash/PSRAM/pins |

For the public MVP, the best balance is:

```text
WROOM dev board for reliability,
XIAO ESP32S3 for compact DIY builds,
generic SuperMini only for people comfortable debugging board quirks.
```

## 5. Microphone Requirements

The microphone must be digital I2S.

Required:

- I2S output.
- 24-bit audio in 32-bit frame or compatible ESP32-S3 I2S capture.
- 3.3 V operation or breakout with 3.3 V support.
- Pins for:
  - `SCK` / `BCLK`;
  - `WS` / `LRCLK`;
  - `SD` / `DOUT`;
  - `L/R` or channel select;
  - `VDD`;
  - `GND`.

Do not buy for this project:

- Analog microphone modules like MAX4466/MAX9814.
- USB microphones.
- PDM-only microphones unless firmware support is explicitly added later.
- Microphone modules that do not expose I2S pins.

## 6. Microphone Options

### Option A - ICS-43434 I2S breakout

Official v0.1 microphone target.

Search on AliExpress:

- `ICS-43434 I2S microphone`
- `ICS43434 I2S microphone module`
- `Adafruit ICS-43434 I2S microphone`

Why it is recommended:

- 24-bit I2S microphone.
- Better published SNR than INMP441.
- Strong official target for voice capture quality.
- Very small price delta when a proper breakout is available.
- Compatible with the existing `I2S -> DSP -> USB` architecture.

What to check in listing photos:

- Pins labeled `SCK`/`BCLK`, `WS`/`LRCLK`, `SD`/`DOUT`, `SEL`/`L/R`, `GND`, `VDD`.
- 3.3 V support.
- Prefer modules with header pins included or clearly solderable pads.
- Prefer full breakouts with local decoupling/filtering around the microphone.
- Avoid bare MEMS chips unless you are making a PCB.

Verdict:

```text
Recommended for v0.1 public build.
```

### Option B - INMP441 I2S module

Common fallback.

Search on AliExpress:

- `INMP441 I2S microphone module ESP32`
- `INMP441 MEMS microphone I2S`
- `INMP441 omnidirectional microphone module`

Why it remains useful:

- Very common on AliExpress.
- Cheap.
- 24-bit I2S microphone.
- Plenty of ESP32 examples exist.
- Useful for fallback bring-up and comparison testing.

Caveats:

- Lower published SNR than ICS-43434.
- More sensitive to board/module quality and supply noise than the preferred target.
- Fallback status means performance caveats must be measured before declaring acceptance.

Verdict:

```text
Supported fallback if ICS-43434 is unavailable.
```

### Option C - SPH0645LM4H-B I2S breakout

Acceptable alternative.

Search on AliExpress:

- `SPH0645 I2S microphone`
- `SPH0645LM4H I2S MEMS microphone`

Why it is useful:

- Reputable I2S MEMS microphone family.
- Common in hobby breakout boards.

Caveats:

- Some builders report timing/alignment quirks depending on driver setup.
- Not the default PRD target.

Verdict:

```text
Acceptable if ICS-43434 and INMP441 are unavailable, but not the first recommendation.
```

### Option D - MSM261S4030H0 I2S module

Budget alternative.

Search on AliExpress:

- `MSM261S4030H0 I2S microphone`
- `Sipeed I2S microphone`
- `DFRobot I2S microphone module`

Why it may be useful:

- Often sold as small I2S modules.
- Cheap and easy to wire.

Caveats:

- Typically lower audio specs than the better TDK/Knowles parts.
- Needs validation before it becomes an official supported default.

Verdict:

```text
Budget fallback, not the reference microphone.
```

### Option E - TDK T5848

Best-looking spec, future PCB candidate.

Search on AliExpress:

- `T5848 I2S microphone`
- `TDK T5848 microphone`

Why it matters:

- High-end I2S MEMS part.
- Published high SNR and high acoustic overload point.

Caveats:

- Usually sold as a bare component or through electronics distributors.
- Not a normal AliExpress beginner module.
- Better suited to a future PCB than the public MVP.

Verdict:

```text
Future premium PCB candidate, not MVP public BoM.
```

## 7. Public MVP Wiring

Default firmware pinout:

| ESP32-S3 Signal | Default GPIO | ICS/INMP441-style Mic Pin |
| --- | ---: | --- |
| I2S BCLK | GPIO4 | `SCK` / `BCLK` |
| I2S WS | GPIO5 | `WS` / `LRCLK` |
| I2S DATA IN | GPIO6 | `SD` / `DOUT` |
| 3.3 V | 3V3 | `VDD` |
| Ground | GND | `GND` |
| Left/right select | GND initially | `SEL` / `L/R` |

If using XIAO ESP32S3:

| Mic Pin | XIAO Pin | ESP32-S3 GPIO |
| --- | --- | ---: |
| `SCK` / `BCLK` | D3 | GPIO4 |
| `WS` / `LRCLK` | D4 | GPIO5 |
| `SD` / `DOUT` | D5 | GPIO6 |
| `VDD` | 3V3 | 3.3 V |
| `GND` | GND | GND |
| `L/R` | GND | Left channel |

Notes:

- Keep I2S wires short.
- Start with `L/R` tied to GND.
- If audio is silent, test tying `L/R` to 3V3 and/or switch channel selection in firmware.
- Do not use GPIO19/GPIO20 for the microphone; those are for native USB.
- The physical configuration button uses a separate spare GPIO wired to GND, with firmware/internal pull-up expected. It must not use GPIO19/20, GPIO4/5/6, or a risky boot strap. The exact default GPIO is still TBD until the reference board is validated.

## 8. Breadboard Build

Breadboard works for the MVP.

Use it for:

- First boot.
- Pin verification.
- I2S capture test.
- Firmware development.
- Quick microphone swaps.

Rules:

- Use short jumpers.
- Keep BCLK/WS/DATA close together and short.
- Avoid long flying wires.
- Do not treat random noise or dropouts as firmware bugs until wiring is shortened.

Minimal breadboard BoM:

| Qty | Item | Search Term |
| ---: | --- | --- |
| 1 | ESP32-S3 board | `ESP32-S3 WROOM N16R8 USB C` |
| 1 | I2S mic module | `ICS-43434 I2S microphone breakout` |
| 1 optional | Fallback I2S mic module | `INMP441 I2S microphone module` |
| 1 | Physical configuration button | `6x6 tactile push button` |
| 1 | Breadboard | `solderless breadboard 400 tie points` |
| 1 set | Jumpers | `dupont jumper wire female male` |
| 1 | USB data cable | `USB C data cable short` |

## 9. DIY Double-Sided Perfboard Build

This should be on the public roadmap immediately after breadboard proof.

Search on AliExpress:

- `double side prototype PCB 2.54mm`
- `double sided perfboard plated through hole`
- `prototype PCB 5x7 cm 2.54mm`
- `2.54mm female pin header`
- `30AWG wire wrap wire`

Recommended layout:

```text
[USB edge of ESP32-S3 board] -> points outward
[I2S mic] -> placed at board edge, acoustic port facing outside
[short wires] -> underside of perfboard
[GND] -> nearby return path for microphone
```

Perfboard BoM:

| Qty | Item | Notes |
| ---: | --- | --- |
| 1 | Double-sided plated-through perfboard | 5x7 cm or smaller is enough |
| 1 | ESP32-S3 board | Full-size or compact |
| 1 | I2S mic module | Prefer ICS-43434 first; INMP441 is fallback |
| 1 | Physical configuration button | Small tactile button for temporary AP window |
| 2 | Female header strips | Socket the ESP32-S3 board if possible |
| 1 | Small female/header strip for mic | Lets the mic be replaced |
| 1 set | 30 AWG wire or short solid-core wire | For underside wiring |
| 4 | Nylon standoffs or adhesive feet | Mechanical stability |
| 1 | Small enclosure | Optional but recommended after firmware works |

Perfboard rules:

- Put the microphone near an edge.
- Do not bury the mic under the ESP32-S3 board.
- Do not put the mic directly beside the USB connector or regulator.
- Route I2S wires short and direct.
- Add a small strain relief for USB if the board will be handled often.
- Leave access to BOOT and RESET.

## 10. What Not To Buy

Avoid:

- `ESP32-C3 SuperMini`.
- `ESP32-S3` listings whose photo says C3/C6.
- Boards with no pinout image.
- Boards with power-only USB.
- Boards that do not expose enough GPIOs for I2S.
- Analog microphone modules.
- PDM microphone modules for the MVP.
- Camera boards unless you specifically want a larger board for another reason.
- Bare MEMS microphone chips unless you are making a PCB.

## 11. AliExpress Catalog Checklist

When comparing listings, confirm:

- The title and silkscreen both say ESP32-S3.
- The board has USB data, not only charging.
- There is a pinout image.
- GPIO4, GPIO5, GPIO6 are accessible, or you are comfortable changing firmware pin config.
- The board has BOOT/RESET access.
- The mic module exposes I2S pins.
- Reviews include real buyer photos.
- Buy at least one spare microphone if possible; cheap MEMS modules are easy to damage.

## 12. Recommended Public BoM

The clean public recommendation is:

| Qty | Item | Public Recommendation |
| ---: | --- | --- |
| 1 | MCU board | ESP32-S3-WROOM-1 N8R8/N16R8 USB-C dev board |
| 1 | Microphone | ICS-43434 I2S microphone breakout |
| 1 optional | Fallback microphone | INMP441 I2S microphone module |
| 1 | Physical configuration button | Small tactile button |
| 1 | USB cable | Short USB data cable |
| 1 | Prototype base | Breadboard first, then double-sided perfboard |
| 1 set | Wiring | Short Dupont jumpers or 30 AWG solder wire |
| 1 set | Headers | 2.54 mm female headers/sockets |
| 1 | Enclosure | Optional for firmware MVP, recommended later |

Compact public recommendation:

| Qty | Item | Public Recommendation |
| ---: | --- | --- |
| 1 | MCU board | Seeed Studio XIAO ESP32S3 |
| 1 | Microphone | ICS-43434 I2S microphone breakout |
| 1 optional | Fallback microphone | INMP441 I2S microphone module |
| 1 | Physical configuration button | Small tactile button |
| 1 | Prototype base | Double-sided perfboard |
| 1 set | Wiring | Short soldered wires |
| 1 | USB cable | Short USB-C data cable |

## 13. References

Official / technical references:

- Espressif ESP32-S3-DevKitC-1 user guide: https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html
- Espressif ESP32-S3-WROOM-1/WROOM-1U datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
- ESP32-S3 hardware design checklist: https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html
- Seeed Studio XIAO ESP32S3 pinout/wiki: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
- Waveshare ESP32-S3-Zero wiki: https://www.waveshare.com/wiki/ESP32-S3-Zero
- Waveshare ESP32-S3-Tiny product page: https://www.waveshare.com/esp32-s3-tiny.htm
- TDK INMP441 datasheet reference: https://product.tdk.com/system/files/dam/doc/product/sw_piezo/mic/mems-mic/data_sheet/inmp441.pdf
- TDK ICS-43434 datasheet reference: https://invensense.tdk.com/wp-content/uploads/2016/02/DS-000069-ICS-43434-v1.2.pdf
- Knowles SPH0645LM4H-B datasheet: https://www.knowles.com/docs/default-source/model-downloads/sph0645lm4h-b-datasheet-rev-c.pdf
- TDK T5848 product page: https://invensense.tdk.com/en-us/products/t5848

## 14. Affiliate Disclosure

Some AliExpress links in this document may be affiliate links. If you buy through them, I may earn a small commission at no extra cost to you. This helps support the development and maintenance of PhonemeFree Unplugged.

Using these links is optional. You can search for the same parts directly on AliExpress or buy them from any other supplier.

AliExpress catalog search links:

- ESP32-S3 WROOM N16R8 USB-C: https://www.aliexpress.com/w/wholesale-ESP32%252dS3-WROOM-N16R8-USB-C.html
- ESP32-S3 DevKitC-1 N8R8: https://www.aliexpress.com/w/wholesale-ESP32%252dS3-DevKitC-1-N8R8.html
- Seeed Studio XIAO ESP32S3: https://www.aliexpress.com/w/wholesale-Seeed-Studio-XIAO-ESP32S3.html
- Waveshare ESP32-S3-Zero: https://www.aliexpress.com/w/wholesale-Waveshare-ESP32%252dS3%252dZero.html
- ESP32-S3 SuperMini: https://www.aliexpress.com/w/wholesale-ESP32%252dS3-SuperMini.html
- INMP441 I2S microphone module: https://www.aliexpress.com/w/wholesale-INMP441-I2S-microphone-module.html
- ICS-43434 I2S microphone: https://www.aliexpress.com/w/wholesale-ICS%252d43434-I2S-microphone.html
- SPH0645 I2S microphone: https://www.aliexpress.com/w/wholesale-SPH0645-I2S-microphone.html
- 6x6 tactile push button: https://www.aliexpress.com/w/wholesale-6x6-tactile-push-button.html
- Double-sided perfboard 2.54 mm: https://www.aliexpress.com/w/wholesale-double-sided-perfboard-2.54mm.html
