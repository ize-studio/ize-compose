# Ize Compose v1.4.3

Release date: 2026-06-30

v1.4.3 is a firmware-only GitHub sync fix. The SD browser page is unchanged from v1.4.2 and remains:

```text
/ize_compose/ize_compose_1-4-2.html
```

## Release Status

- Firmware version is `v1.4.3`.
- Web page version remains `1-4-2`.
- The current firmware assets are `izefirmware.bin` and `izefirmware_v1.4.3.bin`.
- No HTML file change is required when updating from v1.4.2.

## Fix

- Fixed GitHub sync when the sync plan includes remote document deletions.
- The delete entries sent to GitHub's tree API now include the required tree `mode` and `type` fields with `sha:null`.
- This fixes the GitHub API error:

```text
GitHub tree create failed 422: Must supply a valid tree.mode
```

## Update Notes

Use the v1.4.3 firmware file and keep the existing v1.4.2 SD web page:

```text
autoupdate/izefirmware.bin
autoupdate/izefirmware_v1.4.3.bin
/ize_compose/ize_compose_1-4-2.html
```
