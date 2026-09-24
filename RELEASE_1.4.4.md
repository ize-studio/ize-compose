# Ize Compose v1.4.4

Release status: current

v1.4.4 is a firmware update focused on update-notice behavior around Wi-Fi and GitHub sync, plus safer document saving around common navigation and sleep paths.

## Status

- Firmware version is `v1.4.4`.
- Web page version remains `1-4-2` unless another web change is added before release.
- The SD browser page remains `/ize_compose/ize_compose_1-4-2.html`.
- The current firmware assets are `izefirmware.bin` and `izefirmware_v1.4.4.bin`.

## Changes

### Automatic release check after network activity

- After Wi-Fi client mode connects successfully, the firmware checks the latest GitHub Release once.
- After GitHub document sync finishes, the firmware checks the latest GitHub Release before turning Wi-Fi off.
- The device menu update notice is updated from those checks, so the menu can show update availability even if the browser page has not opened the update panel.

### Automatic document saving

- Before opening another document, the current document is saved first.
- Before creating a new document, the current document is saved first.
- Before Ctrl+L sleep, menu Sleep, or automatic sleep, the current document is saved first.
- While editing in normal typing mode, changed documents are saved automatically every 3 minutes without showing the manual `[Saved!]` status message.

## Update Files

Use the v1.4.4 firmware file and keep the existing v1.4.2 SD web page:

```text
autoupdate/izefirmware.bin
autoupdate/izefirmware_v1.4.4.bin
autoupdate/ize_compose_1-4-2.html
```
