# Ize Compose

> **Repository moved**
>
> This repository is no longer updated. Future updates will be maintained at
> [ize-studio/ize-compose](https://github.com/ize-studio/ize-compose).

Multilingual writing firmware for the [Zerowriter Ink](https://www.zerowriter.org/) (Inkplate 5 V2). Started as a Korean-input firmware, now supports 92 keyboard layouts across dozens of scripts.
(if you are looking for Ize-Ribbon, go to https://github.com/ize-studio/Ize-Ribbon )

Current release: **v1.2.0**

v1.2.0 moves environment settings into the browser-based Properties page, adds an SD-loadable Properties web page, writes a settings backup file to SD, and shows document previews in the network document list.

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
  - ESP32, 800×600 monochrome e-ink display
  - Requires SD card for non-Latin fonts and document storage

---

## Features

**Writing**
- Plain-text editing with cursor navigation
- Phonetic Korean composition (cho/jung/jong jamo assembly)
- Latin accent cycling (e.g., a → á → â → ã → ...)
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
- WiFi (AP mode) document page for uploading/downloading/reading/deleting text
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
| `/ize_compose/property_update.html` | Recommended | External Properties and Update web page. If missing, firmware uses the built-in fallback page. |
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

---

## Device Menu and Web Pages

The main device menu is intentionally small:

`New`, `Save`, `Count`, `Network`, `Sleep`, `Properties`

Use `Network` for the document web page. Use `Properties` for environment settings, firmware update, font upload, and image upload.

Both web modes create the device access point and are opened from a browser at:

`http://192.168.4.1/`

The device screen shows the access point, password, and exit hint. Use `Ctrl + Menu` to exit the web mode.

Detailed browser workflow is documented in [WEB_INTERFACE.md](WEB_INTERFACE.md).

---

## Keyboard Layouts (92)

Dvorak, QWERTY, 한국어, Shqip, العربية, Հայերեն, Deutsch (AT/DE/CH), Azərbaycanca, Беларуская, Nederlands (BE/NL), বাংলা, Bosanski / Босански, Português (BR/PT), Български, Français (CA/FR/CH), Català, Hrvatski, Čeština, Dansk, देवनागरी, Eesti, ኢትዮጵያ, Føroyskt, Suomi, Georgian, Ελληνικά, ગુજરાતી, Hausa, עברית, Magyar, Íslenska, Gaeilge, Italiano, 日本語, ಕನ್ನಡ, Qazaq / Қазақ, ខ្មែរ, Kurdî / کوردی, Кыргызча, ລາວ, Español América, Latviešu, Lietuvių, Lëtzebuergesch, മലയാളം, Malti, Māori, Română (MD) / Молдовеняскэ, Монгол, Crnogorski / Црногорски, မြန်မာ, नेपाली, Македонски, Norsk, پښتو, فارسی, Polski, ਪੰਜਾਬੀ, Română, Русский, Srpski / Српски, සිංහල, Slovenčina, Slovenščina, Español, Kiswahili, Svenska, Тоҷикӣ, தமிழ், తెలుగు, ไทย, བོད་སྐད, Türkçe, Українська, English UK, اردو, Oʻzbek / Ўзбек, Tiếng Việt, Cymraeg

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
| ESP32 BLE Keyboard | 0.3.2 | PlatformIO registry |
| Adafruit GFX Library | 1.12.6 | PlatformIO registry |
| Adafruit BusIO | — | PlatformIO registry |

### Build flags

| Flag | Purpose |
|---|---|
| `-DARDUINO_INKPLATE5V2` | Board identification |
| `-DINKPLATE_5V2` | Enables correct code path inside InkplateLibrary |
| `-DBOARD_HAS_PSRAM` | Declares PSRAM presence to ESP-IDF |
| `-mfix-esp32-psram-cache-issue` | Workaround for ESP32 PSRAM cache bug (older silicon) |
| `-DSCREEN_WIDTH=800` / `-DSCREEN_HEIGHT=600` | Display resolution constants |
| `-Os` | Size optimization for OTA-safe firmware size |
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
3. Confirm that the SD card contains `/ize_compose/initial.png`, `/ize_compose/property_update.html`, and `/ize_compose/hwalja/*.bin`.

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
4. Copy `property_update.html` to `/ize_compose/`.
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
      hwalja/
        hwalja_*.bin      - font files to copy to SD card
      settings_backup.json - generated settings backup, if present
  src/                    - clean PlatformIO firmware source
  lib/InkplateLibrary/    - local Inkplate driver required for build
  others/                 - font sources and helper tools, not compiled
  WEB_INTERFACE.md        - browser page usage guide
  INSTALL.md              - install/build notes
  RELEASE_1.1.2.md        - v1.1.2 release notes
  RELEASE_1.1.1.md        - v1.1.1 release notes

src/
  IZEcompose.ino        — main firmware
  jado.h                — keyboard layout definitions and keymaps (92 layouts)
  jeong_eum.h           — Korean composition engine and script engine types
  insoe.h               — text rendering, font selection
  PsramAssets.h         — PSRAM asset loading helpers

lib/
  InkplateLibrary/      — Inkplate driver (local copy)

tools/
  make_fonts.py         — script used to build hwalja_*.bin from font sources
  u8g2/bdfconv.exe      — BDF font converter (used by make_fonts.py)

build/
  fontbuild*/           — intermediate font build artifacts
  noto_fonts/           — source Noto font TTFs used for font building

others/
  *.ttf                 — original/reference font files
  reference-headers/    — unused generated/reference headers, not compiled

platformio.ini          — PlatformIO build config
```

---

## Current Limitations

- Supports only Inkplate 5 V2 (800×600). Other Inkplate boards are not tested.
- Korean cursor movement during mid-syllable composition is not supported.
- Only `.txt` files; no formatting.
- Single document open at a time.
- WiFi and BLE cannot be active simultaneously.
- The sleep image must be exactly 800×600 pixels; other sizes are not handled.

---

## Credits

- [Inkplate Arduino Library](https://github.com/SolderedElectronics/Inkplate-Arduino-library) — Soldered Electronics
- [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX) — Oliver Kraus
- [ESP32 BLE Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) — T-vK
- [SdFat](https://github.com/greiman/SdFat) — Bill Greiman
- [Noto Fonts](https://fonts.google.com/noto) — Google (used for font building; license: SIL OFL 1.1)
- [Zerowriter Ink](https://www.zerowriter.org/) — original hardware

<div class="site-support" aria-label="Support">
        <img class="site-support-logo" src="https://storage.ko-fi.com/cdn/logomarkLogo.png" alt="Ko-fi">
        <p>I build strange little writing tools.<br>If you enjoyed this project, coffee support is welcome.</p>
        <p>Ko-fi: <a href="https://ko-fi.com/dievesa">https://ko-fi.com/dievesa</a></p>
      </div>
