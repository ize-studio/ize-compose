# Ize Compose Web Interface

This page documents the browser pages served by Ize Compose in AP Server mode and Wi-Fi client mode.

Open the browser at:

```text
http://192.168.4.1/
```

The device screen shows the Wi-Fi name or local IP address, PIN when needed, GitHub sync status, and the exit hint. Use `Ctrl + Menu` to leave the web mode.

For `v1.4.1`, the browser setup page can save repository settings for the top-level `Sync` path.

---

## Document Web Page

Enter from the device menu:

```text
Menu -> Network -> WiFi
```

The document web page is for text-file management on the SD card and GitHub sync setup. In Wi-Fi client mode, scan visible networks on the device, choose one with the arrow keys, enter the password if needed, then open the displayed local IP address from another browser on the same network. PIN entry is required to open this browser page and save GitHub settings. Later `Sync` runs from the device without opening the browser again.

### Document list

- Shows documents in newest-number-first order.
- Shows 12 documents per page.
- Each row shows file name, short preview, file size, and actions.
- The preview is read only for the 12 documents on the current page.

### Actions

| Action | What it does |
|---|---|
| `Read` | Opens the selected text file in the browser |
| `Download` | Downloads the selected text file |
| `Delete` | Deletes the selected text file after browser confirmation |
| `Upload Text File` | Saves the uploaded text as the next `doc_N.txt` file |

---

## GitHub Sync Setup

GitHub settings are entered from the unified browser page in Wi-Fi mode.

Enter from the device menu:

```text
Menu -> Network -> WiFi
```

After Wi-Fi connects, open the IP address shown on the device. Enter the device PIN. Scroll to the bottom of the page and use **GitHub Sync**.

Fields:

| Field | Example | Notes |
|---|---|---|
| Owner | `ize-studio` | GitHub account or organization name |
| Repository | `my-private-writing-backup` | Repository name only |
| Branch | `main` | Existing branch to update |
| Document path | `documents` | Folder inside the repository where `.txt` files will be written |
| Token | `github_pat_...` | Token with Contents read/write permission for this repository |

After saving these settings, use:

```text
Menu -> Sync
```

The device will try the last successful Wi-Fi network, upload local text documents to the configured GitHub repository path, show success or failure, turn Wi-Fi off, and return to the menu.

If GitHub settings are missing, or the saved Wi-Fi network is not visible, the device should show an error and fall back to Wi-Fi setup mode.

---

## Unified Browser Page

Enter from the device menu:

```text
Menu -> Network -> WebServer or Menu -> Network -> WiFi
```

For `Network -> WebServer`, enter the 10-digit AP password on the device, join `IZEcompose_FileServer`, open:

```text
http://192.168.4.1/
```

For `Network -> WiFi`, connect the device to the selected Wi-Fi network and open the local IP shown on the device. In both modes, enter the 4-digit browser PIN shown on the device page.

The external web page is loaded from:

```text
/ize_compose/ize_compose_1-4-1.html
```

If that file is missing, the firmware returns an error. Keep this file on the SD card.

### PIN

The PIN field is at the top of the web page. The same PIN is required for:

- Saving settings
- Firmware update
- Font upload
- Image upload

If the PIN is empty or wrong, the change is rejected.

### Environment settings

The Settings & Update tab edits these settings:

| Setting | Control | Notes |
|---|---|---|
| Sleep Timer | Slider | Stored in firmware preferences |
| Text Size | Slider | Display scale |
| Line Space | Slider | Line spacing |
| Character Space | Slider | Letter spacing |
| Speed | Number input | Default `0`; changing can affect typing feel |
| Refresh | Number input | Default `2000`; changing can affect refresh behavior |
| English Keyboard | Radio buttons | Qwerty or Dvorak |
| Language | Scroll list | Selects English or a multi-language layout |

Press `Save` to apply all settings at once.

If no value changed, the page reports that no changes were detected.

### Settings backup

When settings are saved, the firmware writes a backup file:

```text
/ize_compose/settings_backup.json
```

This file is for recovery after reset/reinstall. If preferences are empty and the backup exists, the firmware can restore settings, including the saved GitHub token, and save them back into device preferences.

### Firmware update

Firmware uploads must use this upload filename:

```text
izefirmware.bin
```

The page automatically sends the selected firmware as `izefirmware.bin`.

During update, the firmware stages the file at:

```text
/ize_compose/upload/izefirmware.bin
```

After a successful update, the staged file is removed to avoid repeated update attempts.

### Font and image upload

Font and image uploads no longer require a separate target selector. The filename decides the target.

Examples:

| Uploaded filename | Stored as |
|---|---|
| `initial.png` or any `.png` | `/ize_compose/initial.png` |
| `hwalja_hangul.bin` | `/ize_compose/hwalja/hwalja_hangul.bin` |
| `NanumGothic_hangul.bin` | `/ize_compose/hwalja/hwalja_hangul.bin` |
| `NanumGothic_jamo.bin` | `/ize_compose/hwalja/hwalja_jamo.bin` |
| `hwalja_latin.bin` | `/ize_compose/hwalja/hwalja_latin.bin` |

Supported font suffixes:

```text
latin.bin
hangul.bin
jamo.bin
jp.bin
greek_cyrillic.bin
arabic.bin
indic.bin
sea.bin
misc.bin
```

---

## SD Card File Checklist

Recommended SD card layout:

```text
/ize_compose/
  initial.png
  ize_compose_1-4-1.html

  settings_backup.json        generated by firmware after settings save
  hwalja/
    hwalja_latin.bin
    hwalja_hangul.bin
    hwalja_jamo.bin
    hwalja_jp.bin
    hwalja_greek_cyrillic.bin
    hwalja_arabic.bin
    hwalja_indic.bin
    hwalja_sea.bin
    hwalja_misc.bin
  upload/
    izefirmware.bin           temporary during firmware update
```

`settings_backup.json` and `upload/izefirmware.bin` do not need to be prepared manually for normal use. They are written by the firmware when needed.

## v1.4.1 Unified Page

`/ize_compose/ize_compose_1-4-1.html` is now the single browser page for both WiFi mode and WebServer mode. It has two tabs:

- Documents
- Settings & Update

The previous separate `property_update.html` and `document_server.html` files are kept only as compatibility copies of this page.

Online firmware update is active in this build. The release update flow downloads both firmware and the versioned SD web page when needed. Local file upload remains the supported path for fonts and `initial.png`.