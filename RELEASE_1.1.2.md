# Ize Compose v1.1.2

Release date: 2026-06-01

This is a corrective maintenance release for keyboard state, Caps Lock behavior,
and settings persistence issues found after v1.1.1.

## Highlights

- Fixed Caps Lock status label updates for English layouts.
- Fixed Dvorak Caps Lock input behavior.
- Persisted system menu settings that were previously only changed in RAM.
- Kept the clean install package source and firmware image in sync.

## Fixed

- The English status label now shows lowercase names when Caps Lock is off and
  uppercase names when Caps Lock is on:
  - `[qwerty]` / `[QWERTY]`
  - `[dvorak]` / `[DVORAK]`
- Pressing Caps Lock now marks the status bar dirty so the display updates
  immediately instead of waiting for another redraw.
- Dvorak now applies Caps Lock to alphabetic mapped output while leaving
  punctuation keys unchanged.
- Layout changes now clear and save Caps Lock state in the correct order, so
  stale Caps Lock state does not return after reboot.
- System menu values are now saved and restored:
  - Size
  - Line Sp
  - Char Sp
  - Speed
  - Refresh
  - Auto Sleep
  - Count mode
- The selected document index is honored on boot when it is still valid.
- The install package `src/IZEcompose.ino` was synchronized with the root source
  before building the release firmware.

## Verification

- `platformio run` succeeds from the root project folder.
- `platformio run` succeeds from the clean `Ize-compose/` package folder.
- Root and package `src/IZEcompose.ino` files were compared after sync.
- Release firmware size: `1,862,512 bytes`
- Arabic font size: `18,655 bytes`

SHA-256:

```text
izefirmware.bin
2E1AE07A79BA6B139600359362D33868F32BBEC2675087FD98C7727CFD8C56FE

hwalja_arabic.bin
6B8B38CE2D5CCDA0F1C8ECDB9F2DFD9AEB6B95DB13075BAB8D139C9E898820FE
```

## Upgrade Notes

For the full v1.1.2 update:

1. Upload `firmware/izefirmware.bin`.
2. Copy the contents of `sdcard/` to the SD card root if your SD card resources
   are not already current.
3. Confirm that the device menu shows `v1.1.2` in System Set.
