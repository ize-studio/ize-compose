# Ize Compose v1.1.1

> **Repository moved**
>
> This repository is no longer updated. Future updates will be maintained at
> [ize-studio/ize-compose](https://github.com/ize-studio/ize-compose).

Release date: 2026-05-31

This is a maintenance release focused on Arabic-script rendering and a cleaner
install package layout.

## Hardware Scope

This release is currently intended for **Zerowriter Ink only**.

Even if another ESP32 e-ink device has a display and ESP32 specification that
look compatible, this firmware is not yet a generic target for that hardware.
The display path, board flags, PSRAM assumptions, SD card layout, and especially
the keyboard input path were verified for Zerowriter Ink.

If the keyboard hardware, keyboard controller, wiring, or input method changes,
the input-handling code must be modified and reverified before the firmware is
used on that device.

## Highlights

- Improved Arabic-script font rendering.
- Regenerated `hwalja_arabic.bin` so connected Arabic presentation forms use
  the full 8-pixel cell width and avoid unwanted side gaps.
- Added a clean `Ize-compose/` install package layout with build files, SD card
  files, firmware, and non-build reference files separated.
- Verified the clean package builds successfully with PlatformIO.

## Arabic Font Fix

Arabic, Persian, Urdu, Pashto, and Kurdish Arabic layouts use contextual
presentation forms. In v1.1.0, some connected forms still appeared visually
separated because the generated glyphs were centered inside the fixed 8x16 cell.

v1.1.1 regenerates the Arabic font so Noto Naskh Arabic glyphs are flush across
the cell width. This reduces unwanted left/right blank space and improves
joining between glyphs that should connect.

Updated file:

- `sdcard/ize_compose/hwalja/hwalja_arabic.bin`

## Install Package Layout

The release package is organized as:

- `firmware/izefirmware.bin` - firmware image for upload/update
- `sdcard/` - copy these contents to the SD card root
- `src/` - PlatformIO firmware source
- `lib/InkplateLibrary/` - local Inkplate library required for build
- `others/` - font sources and helper tools not used directly by compilation
- `INSTALL.md` - install/build notes

The device expects SD card resources at:

- `/ize_compose/initial.png`
- `/ize_compose/hwalja/hwalja_latin.bin`
- `/ize_compose/hwalja/hwalja_hangul.bin`
- `/ize_compose/hwalja/hwalja_jamo.bin`
- `/ize_compose/hwalja/hwalja_jp.bin`
- `/ize_compose/hwalja/hwalja_greek_cyrillic.bin`
- `/ize_compose/hwalja/hwalja_arabic.bin`
- `/ize_compose/hwalja/hwalja_indic.bin`
- `/ize_compose/hwalja/hwalja_sea.bin`
- `/ize_compose/hwalja/hwalja_misc.bin`

## Fixed

- Fixed Arabic connected glyphs not visually connecting because of side padding
  in the generated 8x16 bitmap font.
- Fixed the local `platformio.ini` monitor speed line so PlatformIO accepts the
  baud-rate value cleanly.
- Added `min_spiffs.csv` to the clean package so the project does not depend on
  a hidden PlatformIO package path for its partition table.

## Verification

- `platformio run` succeeds from the clean `Ize-compose/` folder.
- Firmware size: `1,861,552 bytes`
- Arabic font size: `18,655 bytes`

SHA-256:

```text
izefirmware.bin
0F7042DCE84DCBFF1CAA4D6765AF252BA3F2C075F301BD7FD4654CF58F7CC5F0

hwalja_arabic.bin
6B8B38CE2D5CCDA0F1C8ECDB9F2DFD9AEB6B95DB13075BAB8D139C9E898820FE
```

## Upgrade Notes

For the full v1.1.1 update:

1. Upload `firmware/izefirmware.bin`.
2. Copy the contents of `sdcard/` to the SD card root.
3. Make sure the updated Arabic font exists at
   `/ize_compose/hwalja/hwalja_arabic.bin`.

If you only need the Arabic rendering fix and are already running v1.1.0, update
`/ize_compose/hwalja/hwalja_arabic.bin` on the SD card.
