# Ize Compose v1.4.0-test

Release date: 2026-06-13

This is a **test build**, not a verified stable release.

Recommended stable version for normal writing: **v1.3.0**

Use this build only if you want to test the new GitHub private repository sync path. If you only need the writing firmware, document server, Properties page, font upload, or image upload, use v1.3.0 until this release is verified on real hardware.

## Test Status

- Firmware builds successfully with direct GitHub sync enabled.
- Bluetooth keyboard mode has been removed to keep the firmware inside the OTA-safe size limit.
- Browser GitHub settings UI exists in the Wi-Fi document page.
- Real-device GitHub sync against a private repository is not yet verified.
- Real-device repeated Wi-Fi connect/sync/disconnect behavior is not yet verified.

## What Changed

- Added experimental direct GitHub document sync.
- Added `Network -> Sync`.
  - Uses the last successful Wi-Fi network.
  - Uploads local `.txt` documents to the configured GitHub repository path.
  - Shows sync success/failure on the device.
  - Turns Wi-Fi off and returns to the menu after the sync flow.
- Changed the Network menu values to:
  - `Off`
  - `Sync`
  - `WiFi`
  - `Server`
- Removed Bluetooth keyboard mode and the `ESP32 BLE Keyboard` dependency.
- Kept `Network -> WiFi` for Wi-Fi client document-server mode.
- Kept `Network -> Server` for AP document-server mode at `http://192.168.4.1/`.
- Kept `Properties` as the AP-based settings/update page.
- Kept external web pages on SD card:
  - `/ize_compose/document_server.html`
  - `/ize_compose/property_update.html`
- Kept image loading through the PNG buffer path used by the firmware.
- Kept the reduced Inkplate image path build flag:
  - `-DIZE_INKPLATE_MINIMAL_IMAGE=1`

## Required SD Card Files

These files should be on the SD card:

```text
/ize_compose/property_update.html
/ize_compose/document_server.html
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

`settings_backup.json` and `upload/izefirmware.bin` do not need to be prepared manually.

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

8. If GitHub asks whether to add a README, either choice is acceptable. The device sync writes document files under the configured document path.

### 2. Choose a document folder

Pick a folder path inside the repository. Recommended:

```text
documents
```

The device will write files like:

```text
documents/doc_1.txt
documents/doc_2.txt
documents/doc_3.txt
```

Use a simple folder name first. For the first test, start with plain ASCII letters, numbers, `_`, `-`, and `/` so path debugging stays simple.

### 3. Create a GitHub token

The device needs a token because the repository is private and the ESP32 must write files through the GitHub API.

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

Do not commit this token into a repository. The device stores it in firmware preferences. The settings backup intentionally does not need to be treated as the token source.

### 4. Enter GitHub settings on Ize Compose

1. Install this firmware.
2. Make sure `/ize_compose/document_server.html` exists on the SD card.
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

### 5. Run sync from the device

After GitHub settings have been saved once:

1. Open the device menu.
2. Select:

   ```text
   Network -> Sync
   ```

3. The device should connect to the last successful Wi-Fi network.
4. The device should upload local `.txt` documents to the configured repository path.
5. The device should show whether sync completed or failed.
6. The device should turn Wi-Fi off and return to the menu.

### 6. Check the repository

Open the private repository in GitHub and check the configured folder:

```text
documents/
```

Expected result:

- Local text documents from the SD card appear as `.txt` files.
- A new commit appears in the repository history.
- The commit message should identify the Ize Compose sync.

## Troubleshooting

### GitHub repository info missing

The device does not have complete GitHub settings. Re-enter `Network -> WiFi`, open the browser page, and save Owner, Repository, Branch, Path, and Token.

### Saved Wi-Fi not found

The last successful Wi-Fi network is not visible. Re-enter `Network -> WiFi`, connect to the desired network again, then try `Network -> Sync`.

### Token rejected

Check:

- Token is not expired.
- Token belongs to an account that can access the repository.
- Repository access includes the selected private repository.
- Contents permission is **Read and write**.
- Owner and repository names are spelled exactly.

### Sync fails after creating blobs or commits

This is still a test build. Record the exact device message and check whether the repository branch already exists and whether the token can write to it.

## Verification

- `platformio run` succeeds.
- Direct GitHub sync is included in the final firmware map.
- `ESP32 BLE Keyboard` is no longer in the PlatformIO dependency graph.
- PNG buffer image loading remains linked.

Firmware:

```text
firmware/izefirmware.bin
Size: 1,216,800 bytes
SHA-256: BA29F4BFEC6E73DB6BA67B2738BEC3DA375A5BABA32287C09D7D48875CBC9E0B
```

## Upgrade Notes

1. Upload `firmware/izefirmware.bin`.
2. Copy the two browser pages to the SD card:

   ```text
   /ize_compose/property_update.html
   /ize_compose/document_server.html
   ```

3. Keep or restore font files under:

   ```text
   /ize_compose/hwalja/
   ```

4. Reboot after update.
5. Confirm that Properties shows:

   ```text
   v1.4.0-test
   ```

6. For stable writing use, return to **v1.3.0** if GitHub sync testing is not needed.
