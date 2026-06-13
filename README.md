# Ize Compose

> **Repository moved**
>
> This repository is no longer updated. Future updates will be maintained at
> [ize-studio/ize-compose](https://github.com/ize-studio/ize-compose).

Multilingual writing firmware for the [Zerowriter Ink](https://www.zerowriter.org/) (Inkplate 5 V2). Started as a Korean-input firmware, now supports 92 keyboard layouts across dozens of scripts.
(if you are looking for Ize-Ribbon, go to https://github.com/ize-studio/Ize-Ribbon )

Current test build: **v1.4.0-test**

Recommended stable version: **v1.3.0**

> **Test build warning**
>
> `v1.4.0-test` is not fully verified yet. It adds the first direct GitHub private-repository sync path and removes Bluetooth keyboard mode to keep the firmware inside the OTA-safe size limit.
> For normal writing use, install **v1.3.0** until the GitHub sync path has been tested on real hardware and real repositories.

v1.3.0 adds Wi-Fi client document-server mode, splits the document web page into an SD-loadable file, keeps Properties in the browser, and preserves the SD settings backup flow.

v1.4.0-test adds the experimental `Network -> Sync` flow for GitHub private repository sync. See [RELEASE_1.4.0_TEST.md](RELEASE_1.4.0_TEST.md) before trying it.

For browser usage details, see [WEB_INTERFACE.md](WEB_INTERFACE.md).

> **Hardware scope warning**
>
> This repository is currently released for **Zerowriter Ink only**.
> Even if another device has an ESP32 and an e-ink display with matching or similar specifications, this code and firmware should not be treated as a general Inkplate/ESP32 writing firmware yet.
>
> In particular, the keyboard input path is written for the current Zerowriter Ink hardware configuration. If the keyboard, keyboard controller, wiring, or input method changes, the input-handling code must be reviewed and modified before use.

---

## Supported Device

- **Zerowriter Ink** (Inkplate 5 V2)
  - ESP32, 800횞600 monochrome e-ink display
  - Requires SD card for non-Latin fonts and document storage

---

## Features

**Writing**
- Plain-text editing with cursor navigation
- Phonetic Korean composition (cho/jung/jong jamo assembly)
- Latin accent cycling (e.g., a ??찼 ??창 ??찾 ??...)
- Right-to-left (RTL) text mode for Arabic-script layouts
- Text search (Ctrl+F)
- Copy / paste (Ctrl+C / Ctrl+V)
- Word and character count (3 display modes)

**Keyboard & Language**
- 92 keyboard layouts selectable from the system menu
- Two independent layout slots: one for English (QWERTY or Dvorak), one for a second language
- 12 script composition engines: Korean, Arabic, Indic scripts, Thai, Myanmar, Khmer, Lao, Tibetan, Sinhala, Ethiopic, Japanese, Hebrew
- RTL layout support: Arabic, Hebrew, Kurdish (Arabic), Pashto, Persian, Urdu

**Files**
- Saves and loads `.txt` files on SD card (`/ize_compose/`)
- File browser (up to 65 files)
- Network modes: Off, Sync, WiFi client document server, and AP Server document page
- Browser document list shows 12 documents per page with a short preview beside each title
- Properties and firmware/font/image updates are handled from the browser-based Properties page

**Display**
- Partial screen update for fast typing feedback
- Configurable full-refresh threshold
- Boot/sleep image loaded from `/ize_compose/initial.png` on SD card
- Sleep mode: Ctrl+L or sleep button; wake with wake button

**Settings and updates**
- Device menu keeps writing/file commands compact: New, Save, Count, Network, Sleep, Properties
- Properties mode opens the browser page for sleep timer, text size, line spacing, character spacing, typing speed, refresh limit, English keyboard, and language selection
- Settings are saved to device preferences and backed up to `/ize_compose/settings_backup.json`
- Firmware uploads use `izefirmware.bin`
- Font and image uploads are routed by filename

---

## SD Card Required Files

The firmware expects support files on the SD card under `/ize_compose/`.

| SD card path | Required | Purpose |
|---|---:|---|
| `/ize_compose/initial.png` | Recommended | Boot/sleep image, 800x600 PNG |
| `/ize_compose/property_update.html` | Required for Properties browser UI | External Properties and Update web page |
| `/ize_compose/document_server.html` | Required for document/GitHub browser UI | External document-management and GitHub settings web page |
| `/ize_compose/hwalja/hwalja_hangul.bin` | Recommended for Korean | Hangul syllable font |
| `/ize_compose/hwalja/hwalja_jamo.bin` | Recommended for Korean | Korean jamo/composition font |
| `/ize_compose/hwalja/hwalja_latin.bin` | Recommended | Full Latin and Latin-extended font |
| `/ize_compose/hwalja/hwalja_jp.bin` | Optional | Japanese Hiragana/Katakana |
| `/ize_compose/hwalja/hwalja_greek_cyrillic.bin` | Optional | Greek and Cyrillic |
| `/ize_compose/hwalja/hwalja_arabic.bin` | Optional | Arabic-script layouts |
| `/ize_compose/hwalja/hwalja_indic.bin` | Optional | Indic-script layouts |
| `/ize_compose/hwalja/hwalja_sea.bin` | Optional | Thai, Khmer, Lao, Myanmar, Tibetan |
| `/ize_compose/hwalja/hwalja_misc.bin` | Optional | Ethiopic, Georgian, Armenian, and other scripts |
| `/ize_compose/settings_backup.json` | Generated | Settings backup written by the firmware. Keep it when preserving settings across reset/reinstall. |
| `/ize_compose/upload/izefirmware.bin` | Temporary | Staged firmware file used internally during SD OTA update |

When using files directly from this repository, make sure the final SD card paths match the table above. Font files must end up inside `/ize_compose/hwalja/`.

The repository `sdcard/ize_compose/` folder currently contains the two browser pages. Font binaries may be distributed separately or generated from the font tools; the device still expects them at the paths listed above.

---

## Device Menu and Web Pages

The main device menu is intentionally small:

`New`, `Save`, `Count`, `Network`, `Sleep`, `Properties`

Use `Network` for `Off`, `Sync`, `WiFi`, or `Server`.

- `Off`: turns network services off.
- `Sync`: connects to the last successful Wi-Fi network, runs GitHub document sync, turns Wi-Fi off, and returns to the menu.
- `WiFi`: scans visible Wi-Fi networks, connects as a client, and serves the document/GitHub settings page at the local IP shown on the device.
- `Server`: starts the device access point and serves the document page at `http://192.168.4.1/`.

Use `Properties` for environment settings, firmware update, font upload, and image upload.

AP Web Server and Properties create the device access point and are opened from a browser at:

`http://192.168.4.1/`

The device screen shows the access point, password, and exit hint. Use `Ctrl + Menu` to exit the web mode.

Detailed browser workflow is documented in [WEB_INTERFACE.md](WEB_INTERFACE.md).

---

## Keyboard Layouts (92)

Dvorak, QWERTY, ?쒓뎅?? Shqip, 碼?晩邈磨?馬, ?蘿蘭樂?樂鸞, Deutsch (AT/DE/CH), Az?rbaycanca, ?筠剋逵???克逵?, Nederlands (BE/NL), 逝о┥逝귖┣逝? Bosanski / ?棘?逵戟?克龜, Portugu챗s (BR/PT), ??剋均逵??克龜, Fran챌ais (CA/FR/CH), Catal횪, Hrvatski, 훻e큄tina, Dansk, 西╆쪍西듀ㄸ西약쨽西겯?, Eesti, ?㏇돲??뙲?? F첩royskt, Suomi, Georgian, ?貫貫管館菅觀郭, 夕쀠쳛夕쒉ぐ夕약い奭, Hausa, 鬧?淚?瘻, Magyar, 횒slenska, Gaeilge, Italiano, ?ζ쑍沃? 淅뺖꺼潟띭꺼淅? Qazaq / ?逵鈞逵?, ?곢윊?섂웴?? Kurd챤 / 沕?邈膜?, ???均?鈞?逵, 僊?볏僊? Espa챰ol Am챕rica, Latvie큄u, Lietuvi킬, L챘tzebuergesch, 石?눠石?늅石녀큲, Malti, M훮ori, Rom창n훱 (MD) / ?棘剋畇棘勻筠戟??克?, ?棘戟均棘剋, Crnogorski / 揆?戟棘均棘??克龜, ?쇹솽붳뷘쇹? 西ⓣ쪍西むㅎ西꿋?, ?逵克筠畇棘戟?克龜, Norsk, 毛?魔?, ?碼邈卍?, Polski, 黍むŉ黍쒉㉭黍о?, Rom창n훱, ????克龜橘, Srpski / 鬼?極?克龜, 釋꺺퇁蓆귖톬蓆? Sloven훾ina, Sloven큄훾ina, Espa챰ol, Kiswahili, Svenska, 龜棘念龜克但, 昔ㅰ?昔욈?晳? 析ㅰ콊析꿋콅析쀠콅, 仙꾝툠錫? 嬋뽤슨嬋묂펻嬋╆풊嬋? T체rk챌e, 叫克?逵?戟??克逵, English UK, 碼邈膜?, O軻zbek / ?鈞閨筠克, Ti梳퓆g Vi沼뇍, Cymraeg

---

## Font Files

The firmware has a built-in Latin fallback font. The full Latin font and all non-Latin script fonts are loaded from SD card at boot when the font files are present.

In a release install package, these files should be arranged under:

`Ize-compose/sdcard/ize_compose/hwalja/`

Copy the contents of `Ize-compose/sdcard/` to the root of the SD card. The device expects the font files in `/ize_compose/hwalja/`.

| File | Scripts covered |
|---|---|
| `hwalja_hangul.bin` | Korean (Hangul syllables) |
| `hwalja_jamo.bin` | Korean (Jamo, composition glyphs) |
| `hwalja_latin.bin` | Latin and Latin extended |
| `hwalja_jp.bin` | Japanese (Hiragana, Katakana) |
| `hwalja_greek_cyrillic.bin` | Greek, Cyrillic |
| `hwalja_arabic.bin` | Arabic, Persian, Urdu, Pashto, Kurdish Arabic |
| `hwalja_indic.bin` | Devanagari, Bengali, Gujarati, Kannada, Malayalam, Punjabi, Tamil, Telugu, Sinhala |
| `hwalja_sea.bin` | Thai, Khmer, Lao, Myanmar, Tibetan |
| `hwalja_misc.bin` | Ethiopic, Georgian, Armenian, and others |

Without these files the device still works, but only the built-in Latin fallback font is available.

### v1.1.1 Arabic font update

`hwalja_arabic.bin` was regenerated in v1.1.1 so Arabic presentation forms use the full 8-pixel cell width. This reduces unwanted left/right blank space and improves visual connection between glyphs that should join.

---

## Build Environment

### Platform / Board / Framework

| Item | Value |
|---|---|
| Platform | `espressif32` |
| Board | `esp32dev` + Inkplate 5 V2 build flags |
| Framework | Arduino |
| CPU clock | 240 MHz |
| Upload / monitor speed | 921600 baud |

> `board = esp32dev` is used with manual build flags rather than a dedicated Inkplate board definition. This firmware will not work on a generic ESP32 dev board. The flags, PSRAM assumptions, display path, and keyboard input code are specific to the Zerowriter Ink / Inkplate 5 V2 hardware.

### Libraries

| Library | Version | Source |
|---|---|---|
| InkplateLibrary | 11.0.0 | `lib/` (local, no separate install needed) |
| SdFat | 2.3.1 | PlatformIO registry |
| U8g2_for_Adafruit_GFX | 1.8.0 | PlatformIO registry |
| Adafruit GFX Library | 1.12.6 | PlatformIO registry |
| Adafruit BusIO | ??| PlatformIO registry |

### Build flags

| Flag | Purpose |
|---|---|
| `-DARDUINO_INKPLATE5V2` | Board identification |
| `-DINKPLATE_5V2` | Enables correct code path inside InkplateLibrary |
| `-DBOARD_HAS_PSRAM` | Declares PSRAM presence to ESP-IDF |
| `-mfix-esp32-psram-cache-issue` | Workaround for ESP32 PSRAM cache bug (older silicon) |
| `-DSCREEN_WIDTH=800` / `-DSCREEN_HEIGHT=600` | Display resolution constants |
| `-Os` | Size optimization for OTA-safe firmware size |
| `-DIZE_INKPLATE_MINIMAL_IMAGE=1` | Keeps only the image loading path this firmware uses |
| `-DIZE_ENABLE_DIRECT_GITHUB_SYNC=1` | Builds the experimental direct GitHub sync path |
| `-DIZE_ENABLE_BLE_KEYBOARD=0` | Keeps removed Bluetooth keyboard code out of the build |
| `-D CORE_DEBUG_LEVEL=0` | Suppresses all serial debug output |

### Flash / partition settings

| Item | Value |
|---|---|
| Partition table | `min_spiffs.csv` |
| Flash speed | 80 MHz |
| Flash mode | QIO (quad I/O) |

> The clean v1.1.1 package includes `min_spiffs.csv` at the project root so the build does not depend on a hidden PlatformIO package path.

---

## Installation

### Requirements
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Zerowriter Ink (Inkplate 5 V2) with SD card

### Recommended release install

Download or open the `Ize-compose/` package.

1. Upload `Ize-compose/firmware/izefirmware.bin` as the firmware image.
2. Copy the contents of `Ize-compose/sdcard/` to the SD card root.
3. Confirm that the SD card contains `/ize_compose/initial.png`, `/ize_compose/property_update.html`, `/ize_compose/document_server.html`, and `/ize_compose/hwalja/*.bin`.

### Build and flash from source
```bash
git clone <this repo>
cd <repo>
pio run --target upload
```

### SD card setup
1. Format SD card as FAT32.
2. Create `/ize_compose/` and `/ize_compose/hwalja/` on the SD card if they are not already present.
3. Copy `initial.png` to `/ize_compose/`.
4. Copy `property_update.html` and `document_server.html` to `/ize_compose/`.
5. Copy `hwalja_*.bin` font files into `/ize_compose/hwalja/`.
6. Keep `settings_backup.json` if it exists and you want to preserve settings across reset/reinstall.

### Firmware OTA update (WiFi)
1. Open `Menu -> Properties`.
2. Enter the 4-digit PIN on the device.
3. Connect to the device access point from a PC or phone.
4. Open `http://192.168.4.1/` in a browser.
5. Select the firmware file. The upload page sends it as `izefirmware.bin`.
6. Wait for the device update/reboot flow to finish.

For document transfer and Properties page usage, see [WEB_INTERFACE.md](WEB_INTERFACE.md).

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| Ctrl+Space | Toggle Korean / English mode |
| Ctrl+L | Sleep (shows boot image) |
| Ctrl+F | Text search |
| Ctrl+C | Copy all text to clipboard |
| Ctrl+V | Paste clipboard |
| Space (accent cycling) | Cycle diacritic variants for last character |

---

## Repository Structure

```
Ize-compose/
  firmware/
    izefirmware.bin       - release firmware image
  sdcard/
    ize_compose/
      initial.png         - boot/sleep image
      property_update.html - external Properties web page
      document_server.html - external document web page
      hwalja/
        hwalja_*.bin      - font files to copy to SD card
      settings_backup.json - generated settings backup, if present
  src/                    - clean PlatformIO firmware source
  lib/InkplateLibrary/    - local Inkplate driver required for build
  others/                 - font sources and helper tools, not compiled
  WEB_INTERFACE.md        - browser page usage guide
  INSTALL.md              - install/build notes
  RELEASE_1.4.0_TEST.md   - v1.4.0-test release notes
  RELEASE_1.1.2.md        - v1.1.2 release notes
  RELEASE_1.1.1.md        - v1.1.1 release notes

src/
  IZEcompose.ino        ??main firmware
  jado.h                ??keyboard layout definitions and keymaps (92 layouts)
  jeong_eum.h           ??Korean composition engine and script engine types
  insoe.h               ??text rendering, font selection
  PsramAssets.h         ??PSRAM asset loading helpers

lib/
  InkplateLibrary/      ??Inkplate driver (local copy)

tools/
  make_fonts.py         ??script used to build hwalja_*.bin from font sources
  u8g2/bdfconv.exe      ??BDF font converter (used by make_fonts.py)

build/
  fontbuild*/           ??intermediate font build artifacts
  noto_fonts/           ??source Noto font TTFs used for font building

others/
  *.ttf                 ??original/reference font files
  reference-headers/    ??unused generated/reference headers, not compiled

platformio.ini          ??PlatformIO build config
```

---

## Current Limitations

- Supports only Inkplate 5 V2 (800횞600). Other Inkplate boards are not tested.
- Korean cursor movement during mid-syllable composition is not supported.
- Only `.txt` files; no formatting.
- Single document open at a time.
- Bluetooth keyboard mode was removed in v1.4.0-test to make room for direct GitHub sync.
- The sleep image must be exactly 800횞600 pixels; other sizes are not handled.

---

## Credits

- [Inkplate Arduino Library](https://github.com/SolderedElectronics/Inkplate-Arduino-library) ??Soldered Electronics
- [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX) ??Oliver Kraus
- [SdFat](https://github.com/greiman/SdFat) ??Bill Greiman
- [Noto Fonts](https://fonts.google.com/noto) ??Google (used for font building; license: SIL OFL 1.1)
- [Zerowriter Ink](https://www.zerowriter.org/) ??original hardware

<div class="site-support" aria-label="Support">
        <img class="site-support-logo" src="https://storage.ko-fi.com/cdn/logomarkLogo.png" alt="Ko-fi">
        <p>I build strange little writing tools.<br>If you enjoyed this project, coffee support is welcome.</p>
        <p>Ko-fi: <a href="https://ko-fi.com/dievesa">https://ko-fi.com/dievesa</a></p>
      </div>
