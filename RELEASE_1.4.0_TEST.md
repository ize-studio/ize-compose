# Ize Compose v1.4.0-test

Release date: 2026-06-13

This is a **test build**, not a verified stable release.

Recommended stable version for normal writing: **v1.3.0**

Use this build only if you want to test the new GitHub private repository sync path. If you only need the stable writing firmware, document server, font upload, or image upload, use v1.3.0 until this release is verified on real hardware.

## Test Status

- Firmware builds successfully with direct GitHub sync enabled.
- Bluetooth keyboard mode has been removed to keep the firmware inside the OTA-safe size limit.
- Browser GitHub settings UI exists in the Wi-Fi document page.
- Real-device GitHub sync against a private repository is still a test path.
- The repository-side `documents/index.html` file manager was removed. Private GitHub HTML files cannot be opened as a live web page without downloading them first, so online file management is not part of this release.

## What Changed

- Added experimental direct GitHub document sync.
- Added top-level `Sync`.
  - Uses the last successful Wi-Fi network.
  - Uploads local `.txt` documents to the configured GitHub repository path.
  - Downloads newer GitHub edits for existing local documents.
  - Deletes remote-only `docNNNN.txt` files instead of downloading them.
  - Restores local documents if the matching GitHub file was deleted.
  - Shows sync success/failure on the device.
  - Turns Wi-Fi off and returns to the menu after the sync flow.
- Changed the Network menu values to:
  - `Off`
  - `WiFi`
  - `WebServer`
- Removed Bluetooth keyboard mode and the `ESP32 BLE Keyboard` dependency.
- Kept `Network -> WiFi` for Wi-Fi client document-server mode.
- Changed `Network -> WebServer` to ask for a 10-digit numeric AP password and serve the same unified browser page at `http://192.168.4.1/`.
- Removed `Properties` as a selectable menu item; settings and update are now in the unified browser page.
- Kept external web pages on SD card:
  - `/ize_compose/ize_compose_1-4-0-test.html`

- Kept image loading through the PNG buffer path used by the firmware.
- Kept the reduced Inkplate image path build flag:
  - `-DIZE_INKPLATE_MINIMAL_IMAGE=1`

## Required SD Card Files

Prepare these files on the SD card:

```text
/ize_compose/ize_compose_1-4-0-test.html
/ize_compose/initial.png
/ize_compose/hwalja/hwalja_latin.bin
/ize_compose/hwalja/hwalja_hangul.bin
/ize_compose/hwalja/hwalja_jamo.bin
```

Recommended additional font files:

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

`settings_backup.json` is written by the firmware as a settings backup. `upload/izefirmware.bin` is an internal staged firmware file used during SD OTA update. Neither file needs to be prepared manually for a normal install.

There is no repository-side `documents/index.html` file in this release.

## GitHub Private Repository Setup

### 1. Create a private repository

1. Open GitHub in a browser.
2. Click **New repository**.
3. Set **Owner** to the account or organization that should own the backup repository.
4. Enter a repository name, for example:

   ```text
   ize-compose-documents
   ```

5. Select **Private**.
6. Create the repository.
7. Keep the default branch name. Most new repositories use:

   ```text
   main
   ```

8. If GitHub asks whether to add a README, either choice is acceptable.

The firmware creates the configured document path when it uploads documents. GitHub does not store empty folders, so you do not need to prepare an empty `documents` folder manually.

### 2. Create a GitHub token

The device needs a token because the repository is private and GitHub requires API authentication for reading and writing repository contents.

Recommended token type:

```text
Fine-grained personal access token
```

Steps:

1. Open GitHub.
2. Go to **Settings**.
3. Open **Developer settings**.
4. Open **Personal access tokens**.
5. Choose **Fine-grained tokens**.
6. Create a new token.
7. Set the token owner to the same account or organization that owns the repository.
8. Under **Repository access**, choose **Only select repositories**.
9. Select the private repository created above.
10. Under **Repository permissions**, set:

    ```text
    Contents: Read and write
    Metadata: Read-only
    ```

11. Generate the token.
12. Copy the token immediately. GitHub will not show it again.

### 3. Enter GitHub settings on Ize Compose

1. Install this firmware.
2. Make sure this SD card file exists:

   ```text
   /ize_compose/ize_compose_1-4-0-test.html
   ```

3. On the device, open:

   ```text
   Menu -> Network -> WiFi
   ```

4. Select a visible Wi-Fi network.
5. Enter the Wi-Fi password if required.
6. After connection, the device shows a local IP address.
7. Open that IP address from a computer or phone on the same Wi-Fi network.
8. Enter the PIN shown on the device.
9. Scroll to **GitHub Sync**.
10. Fill in:

    ```text
    Owner:       your GitHub account or organization
    Repository:  repository name only
    Branch:      main
    Path:        documents
    Token:       the token copied from GitHub
    ```

11. Click **Save GitHub Settings**.
12. Leave Wi-Fi mode with `Ctrl + Menu`.

### 4. Run sync from the device

After GitHub settings have been saved once:

1. Open the device menu.
2. Select:

   ```text
   Sync
   ```

3. The device should connect to the last successful Wi-Fi network.
4. The device should sync local `.txt` documents with the configured repository path.
5. The device should show whether sync completed or failed.
6. The device should turn Wi-Fi off and return to the menu.

## GitHub Sync Rules

- Existing `docNNNN.txt` files may be edited directly on GitHub.
- If the GitHub copy of an existing document is newer, the device downloads it.
- New documents should be created on the device or uploaded through the device Wi-Fi/WebServer unified page as text files.
- Files created only on GitHub are removed on the next sync.
- Files deleted only on GitHub are restored on the next sync if the SD card still has them.
- Device-side deletion remains available and keeps its PIN/confirmation flow. Use device-side deletion when a document should be removed from both the SD card and GitHub.
- There is no browser-side online file manager in the private GitHub repository.

## Troubleshooting

### GitHub repository info missing

The device does not have complete GitHub settings. Re-enter `Network -> WiFi`, open the browser page, and save Owner, Repository, Branch, Path, and Token.

### Saved Wi-Fi not found

The last successful Wi-Fi network is not visible. Re-enter `Network -> WiFi`, connect to the desired network again, then try `Sync`.

### Token rejected

Check:

- Token is not expired.
- Token belongs to an account that can access the repository.
- Repository access includes the selected private repository.
- Contents permission is **Read and write**.
- Owner and repository names are spelled exactly.

### Files created on GitHub disappear

This is expected. The device owns document creation and numbering. Create new documents on the device or upload `.txt` files through the device Wi-Fi/WebServer unified page.

### Files deleted on GitHub come back

This is expected if the SD card still has the matching document. Delete from the device when the deletion should sync to both sides.

## Verification

- `platformio run` succeeds.
- Direct GitHub sync is included in the final firmware map.
- `ESP32 BLE Keyboard` is no longer in the PlatformIO dependency graph.
- PNG buffer image loading remains linked.
- No repository-side `documents/index.html` file is included.

Firmware:

```text
firmware/izefirmware.bin
Size: 1,261,376 bytes
SHA-256: BFAED6F166C1CBD9F902027A1C3584CB85EFB1FD66CBBAB5E2D630CE7E9B40CF
```

## Upgrade Notes

1. Upload `firmware/izefirmware.bin`.
2. Copy the unified browser page to the SD card:

   ```text
   /ize_compose/ize_compose_1-4-0-test.html

   ```

3. Keep or restore font files under:

   ```text
   /ize_compose/hwalja/
   ```

4. Reboot after update.
5. Confirm that the Settings & Update tab shows:

   ```text
   v1.4.0-test
   ```

6. For stable writing use, return to **v1.3.0** if GitHub sync testing is not needed.
## v1.4.0-test unified web change

- Replaced the two separate SD browser pages with `/ize_compose/ize_compose_1-4-0-test.html`.
- The unified page has Documents and Settings & Update tabs and is served in both WiFi mode and WebServer mode.
- The main menu now starts with `Sync`; `Network` only selects `Off`, `WiFi`, or `WebServer`.
- `Properties` was removed as a selectable menu item. The firmware version remains display-only at the bottom of the menu.
- WebServer mode now asks for a 10-digit numeric access-point password before starting.
- WebServer mode hides the password after a client connects and shuts down automatically after connected clients disappear for more than 2 seconds.
- Online asset backup/restore and online firmware update buttons now call firmware endpoints. Online update downloads the required release firmware and versioned SD web page before OTA starts.
## Online update and asset backup completion

- Online update now checks the latest GitHub release.
- It compares both firmware version and SD web-page asset name.
- If the release web page differs, the device downloads the new versioned web page first.
- Firmware OTA starts only after the firmware file and required SD web-page file downloads have both completed.
- GitHub asset backup/restore includes the versioned web page file in addition to fonts and `initial.png`.