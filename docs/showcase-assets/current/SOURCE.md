# Current UI captures

Captured on 2026-10-26 after rebuilding the working tree with `build.cmd`.

- `desktop.png`: native Qt documentation capture at 1.5x scale, from `runtime/Azec.exe --dry-run --capture-ui=...` through `tests/hidden_process.py` in a separate noninteractive Windows station.
- `phone-screen.png`: the current embedded phone page, paired to the isolated app fixture in headless Chrome at 440 × 956 CSS pixels and 3x device scale. This is browser emulation, not a physical-phone capture.
- `phone-mockup.png`: that capture inside the existing iPhone presentation frame and screen mask; see [frame provenance](../devices/SOURCE.md).
- `desktop-transparent.png`: the desktop capture with nonopaque surface alpha multiplied by 0.8; fully opaque text and control pixels are preserved. This is a presentation-only adjustment.

Both interfaces show the same isolated Ash primary example: 12.3 vertical, -2.4 horizontal, 75 ms shot delay, Rapid fire and Humanize enabled, secondary profile enabled, and Recoil off. No personal profiles, pairing codes, links, or QR images were used in the captures. All fixture process trees were closed after capture.

The README's `docs/showcase-11J.png` uses these actual captures. The matching background is `%USERPROFILE%/Desktop/Code-Dev/Canav/backgrounds/meadow.png`. The composition retains the existing device placement and uses `assets/logo.svg` as the first letter of Azec, above the centered `DESKTOP + PHONE` subtitle. Application opacity and behavior are unchanged.
