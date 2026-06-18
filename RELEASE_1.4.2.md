# Ize Compose v1.4.2

Release date: 2026-06-17

Firmware v1.3.x supports browser-based manual firmware update by selecting a `.bin` file on the WebServer page and uploading it to the device. Starting with v1.4.0, Ize Compose can automatically handle the full Online Update flow from GitHub Release asset download through firmware update, so install this build first and try Online Update when the official v1.4.2 release is published on June 28, 2026. The v1.4.2 firmware and web page are already built and staged in `autoupdate/`; if you want to install v1.4.2 before the official release is published, download the files from `autoupdate/` and use them manually. Use `autoupdate/izefirmware.bin` or `autoupdate/izefirmware_v1.4.2.bin` as the firmware file, and copy `autoupdate/ize_compose_1-4-2.html` to `/ize_compose/` on the SD card.

This release updates the firmware and SD browser page to v1.4.2. It keeps the one-button online update flow, adds browser-triggered GitHub sync in Wi-Fi mode, and fixes the device menu language label so it shows the language selected in settings instead of the temporary input mode.

## Release Status

- Firmware version is `v1.4.2`.
- Web page version is `1-4-2`.
- The SD browser page for this release is `/ize_compose/ize_compose_1-4-2.html`.
- The current release firmware assets are `izefirmware.bin` and `izefirmware_v1.4.2.bin`.
- Online update still downloads both the firmware asset and the versioned SD browser page from GitHub Release assets before OTA starts.

## Changes Since v1.4.1

### Browser GitHub sync

- Added a `Sync Now` button to the unified browser page.
- Browser-triggered GitHub sync is available only in Wi-Fi mode after PIN authentication.
- The browser shows sync progress in a terminal-style log while the device performs the same GitHub document sync flow.
- WebServer/AP mode keeps GitHub sync and online update actions unavailable because those actions require an external internet connection.
- GitHub repository settings can still be saved from the browser page.

### Online update status

- The firmware stores release update availability after checking GitHub Release status.
- The online update flow still shows the device-side download warning before OTA:

```text
Downloading update files.
Do not close browser or power off.
```

### Device menu language label

- The device menu header now shows the language selected in settings.
- Pressing `Ctrl+Space` to switch temporarily to English input no longer changes that menu label to English/Qwerty/Dvorak.
- This makes the selected language visible without opening the browser settings page.

### Release assets

- Added the versioned SD browser page:

```text
/ize_compose/ize_compose_1-4-2.html
```

- Added the versioned firmware asset:

```text
izefirmware_v1.4.2.bin
```

## Required SD Card Files

Prepare these files on the SD card:

```text
/ize_compose/ize_compose_1-4-2.html
/ize_compose/initial.png
/ize_compose/hwalja/hwalja_latin.bin
/ize_compose/hwalja/hwalja_hangul.bin
/ize_compose/hwalja/hwalja_jamo.bin
```

Recommended additional font files remain unchanged:

```text
/ize_compose/hwalja/hwalja_jp.bin
/ize_compose/hwalja/hwalja_greek_cyrillic.bin
/ize_compose/hwalja/hwalja_arabic.bin
/ize_compose/hwalja/hwalja_indic.bin
/ize_compose/hwalja/hwalja_sea.bin
/ize_compose/hwalja/hwalja_misc.bin
```

Generated files:

```text
/ize_compose/settings_backup.json
/ize_compose/upload/izefirmware.bin
```

`settings_backup.json` is written by the firmware as a settings backup. `upload/izefirmware.bin` is an internal staged firmware file used during SD OTA update and is removed after successful or failed OTA handling.

## Update Notes

If the device is already running v1.3.x, v1.4.0, or v1.4.1 with the browser update page available, use `Network -> WebServer` or `Network -> WiFi`, open the browser page, and run Online Update from the Settings & Update tab.

If the installed firmware does not provide that browser update page, disconnect the keyboard cable and connect the device by USB for a wired firmware upload.

## Emulator

The Ize Compose Emulator is available on the `emulator` branch for trying the screen, keyboard, menu flow, language selection, document handling, and simulated browser interface before installing firmware on hardware.

Guide:

```text
https://github.com/ize-studio/ize-compose/blob/emulator/ize-compose-emulator.md
```
