# Ize Compose v1.4.1

Release date: 2026-06-15

This release collects the fixes made after the first v1.4.0 release and updates the firmware/web-page version to v1.4.1.

## Release Status

- Firmware builds successfully with direct GitHub sync enabled.
- Online update keeps the one-button GitHub Release update flow for firmware and the versioned SD browser page.
- The SD browser page for this release is `/ize_compose/ize_compose_1-4-1.html`.
- The current release firmware assets are `izefirmware.bin` and `izefirmware_v1.4.1.bin`.

## Changes Since v1.4.0

### Online update

- Fixed GitHub Release asset download handling so the firmware and versioned SD web page are downloaded before OTA starts.
- Added clearer update progress on the device while release files are downloading.
- Removed the failed staged firmware file after OTA failure so a bad or zero-byte `/ize_compose/upload/izefirmware.bin` cannot keep retrying.
- Kept the release web page filename versioned so the page currently open in the browser is not overwritten during the update.

### GitHub sync

- Fixed GitHub blob SHA calculation so unchanged documents are not uploaded again only because the local SHA was calculated differently from GitHub.
- Fixed sync state handling for empty documents and remote deletes.
- Added a sync count screen before transfer: Push, Pull, and Delete counts are shown before the device starts writing changes.
- Enforced the ESP32 sync load limit: upload + download must be under 128 files.
- Shows the sync-limit error for 5 seconds and sync-failure messages for 3 seconds.

### WebServer and Wi-Fi screen

- Removed the hardcoded `GitHub not connected` line from the Wi-Fi Web Server screen.
- The device now shows GitHub repository/token setup status instead of a stale fixed message.

## Required SD Card Files

Prepare these files on the SD card:

```text
/ize_compose/ize_compose_1-4-1.html
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

If the device is already running v1.3.x or v1.4.0 with the browser update page available, use `Network -> WebServer` or `Network -> WiFi`, open the browser page, and run Online Update from the Settings & Update tab.

If the installed firmware does not provide the browser update page, disconnect the keyboard cable and connect the device by USB for a wired firmware upload.