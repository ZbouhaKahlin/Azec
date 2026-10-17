# Project instructions

## Scope and architecture

Azec is a Windows x64 C++17 / Qt 6.8.3 desktop application with an embedded local-network phone interface. Read `README.md` before changing behavior or packaging.

- `src/app.cpp` / `src/startup.hpp`: startup, single-instance activation, configuration recovery, and verification dispatch.
- `src/main_window.hpp`: desktop UI, settings, hotkeys, favorites, tray, and dialogs.
- `src/app_style.hpp` / `src/strings.hpp`: styles and localized strings.
- `src/phone_bridge.cpp`: mobile API callbacks and state building.
- `src/profile_transfer.hpp` / `src/profile_transfer.cpp`: complete profile backup validation and import/export.
- `src/ui_widgets.hpp`: custom controls, operator rows, title animation, popup, raised panel, page transitions, and fading scrollbars.
- `src/core.hpp` / `src/core.cpp`: primary binary configuration layout, weapon profiles, and movement arithmetic.
- `src/engine.hpp` / `src/engine.cpp`: synchronized configuration, active weapon selection, and input worker threads.
- `src/profile_store.hpp`: secondary weapon profiles and additional preferences in JSON.
- `src/phone_server.hpp`: paired HTTP server and request validation.
- `src/lan_address.hpp`: Windows adapter discovery and LAN pairing-address selection.
- `assets/phone.html`: self-contained mobile UI, embedded by `assets/operators.qrc`.
- `src/launcher.cpp`: root launcher; runs `runtime/Azec.exe` with the launcher directory as its working directory.
- `tests/phone_test.py`: HTTP integration checks against an isolated dry-run instance.
- `tests/core_checks.cpp` / `tests/ui_checks.cpp`: separate verification code, excluded from the release executable.

## Build and verification

Use PowerShell from the project root. Requires Visual Studio 2022 C++ tools, a Windows SDK, and Qt 6.8.3 MSVC x64. `setup-qt.ps1` installs the pinned Qt kit; `NEON_QT_ROOT` can override its location.

```powershell
.\build.cmd test
```

This builds and deploys the app, creates the root launcher and separate verification executable, and runs core plus isolated HTTP/restart checks. Build intermediates belong in `.build/`; deployed binaries belong in `runtime/`. Exclude `runtime/Azec-checks.exe` from runnable releases.

Run UI checks in an isolated working directory because the checks write fixture configurations:

```powershell
New-Item -ItemType Directory -Force .build\verification | Out-Null
Push-Location .build\verification
& '..\..\runtime\Azec.exe' --ui-self-test
Pop-Location
```

Use separate fixture directories and `QT_SCALE_FACTOR=1` / `1.5` for both display scales. Inspect the JSON report and screenshots. Match testing to the change; UI changes require visual inspection, and profile/network changes require behavior checks. Use `--dry-run` for interactive inspection and browser tests so no mouse events or beeps are generated. Do not run fixture tests from the project root or use them against the user's live configuration.

`python tests/run_checks.py --ui-only` creates fresh fixture directories and runs both scales. Build with `build.cmd test` first; the normal `--ui-self-test` command delegates to the separate verification executable.

For phone checks, start the runtime with `--dry-run --phone-info=phone-info.json` in a fresh fixture directory, then run `python tests/phone_test.py <fixture-directory>`. The info file contains temporary session credentials and must remain local. Shut down test processes started by your work after verification.

Optional browser checks use `python tests/phone_browser_test.py` with Python Playwright and Google Chrome installed. The runner creates an isolated dry-run instance, checks all nine languages at four widths, and stops its own process. Keep physical-phone testing distinct from browser emulation.

## Data and behavior contracts

- Preserve the original 3,734-byte primary configuration layout in `r6_config.bin`. Keep backward compatibility when adding features.
- The operator roster covers all 72 operators in id order: Ace, Alibi, Amaru, Aruni, Ash, Azami, Bandit, Blackbeard, Blitz, Brava, Buck, Capitao, Castle, Caveira, Clash, Deimos, Doc, Dokkaebi, Echo, Ela, Fenrir, Finka, Flores, Frost, Fuze, Glaz, Goyo, Gridlock, Grim, Hibana, Iana, IQ, Jackal, Jager, Kaid, Kali, Kapkan, Lesion, Lion, Maestro, Maverick, Melusi, Mira, Montagne, Mozzie, Mute, Nokk, Osa, Oryx, Pulse, Ram, Rook, Sens, Sentry, Skopos, Smoke, Solis, Striker, Sledge, Tachanka, Thatcher, Thermite, Thorn, Thunderbird, Tubarao, Twitch, Valkyrie, Vigil, Wamai, Warden, Ying, Noor. Keep this order: extend at the end only so existing records never shift.
- `r6_config.bin` serializes only the original 71 records. Primary profiles for operators past the original list (Noor, id 71) and the selected operator persist in `azec_weapons.json` (`primaryProfiles`, `selected`); the binary `selected` field still rejects indexes past the original list. Backups with 71 or 72 records import by name.
- Store secondary profiles, weapon hotkeys, and the phone-access preference in `azec_weapons.json`. Favorites use `azec_ui.json`.
- Fresh configurations and the unpaired phone page default to English. Preserve saved language IDs: Portuguese is 0 and English is 1; pairing follows the desktop's saved language.
- Each operator has a primary profile and an optional independent secondary profile. First enable copies primary settings; disabling preserves secondary edits. Character selection starts on primary.
- Default weapon keys are 1 and 2 and are configurable. Weapon shortcuts must not interrupt text/number editing or Ctrl/Alt/Windows-key shortcuts.
- Preserve fractional accumulation, timing, and SendInput behavior unless the requested change concerns them. Reset fractional movement when the active operator/weapon changes.
- Use Engine synchronization when accessing configuration from worker threads. Keep UI updates on the Qt GUI thread.
- Never replace, commit, or package the user's personal configuration files as defaults.

## UI contracts

- The interface follows GoofyUI (github.com/Azec-Zuckerberg/goofy-ui-library): chunky 2-3 px ink outlines, flat colour, hard offset ink shadows without blur, no gradients or glow. Retain the compact window, 200 px sidebar, custom window controls, rounded corners, the cream `#FFFAEE` canvas with white surfaces, the yellow `#FFDD59` header band, the GoofyUI purple default accent (`#7248D7`), and the stationary raised right panel.
- Use bundled Geist for controls, operator names, and body text, and bundled Fredoka for display headings. The top-left Azec wordmark uses the bundled Press Start 2P pixel font (white pixel lettering with a hard ink shadow) inside the existing header.
- Header icons share 34 px hit targets and centered 20 px vector artwork. Controls never react to mouse hover anywhere (header icons, rows, buttons, sliders, scrollbars, and the phone page stay flat until pressed, focused, or selected; only context-menu items still highlight on hover). Keep Save flat, without a glow, pulse, or checkmark swap; its tooltip confirms success. Keyboard focus draws a 2 px accent outline, not a background tile.
- Save stays neutral ink while saved and uses the current accent while unsaved, including after a failed save; text on the accent is chosen by luminance so it stays readable. Keep unsaved data state independent of temporary feedback. Pairing QR starts obscured, reveals on explicit click/keyboard activation, and conceals on leaving Settings, hiding the window, or changing the link.
- Use 24 px outer window corners, 24 px panel corners, and 10-18 px button/input/card corners. Numbers are yellow mono chips, sliders have lavender pill tracks with fully round white ink-rimmed handles (the slider keeps a 20 px minimum height so the handle is never clipped top or bottom), and switches are GoofyUI face toggles (lavender off, accent on, white knob showing `-` or `:)`). Preserve the existing layout and hit targets.
- Sidebar groups and section headings render as GoofyUI stickers: slightly tilted colour chips (yellow, blue, pink, lime) with 2 px ink outlines.
- Operator rows are 52 px tall with 44 px icons and 14 px Geist names; the selected row is a lavender pill with an ink outline. Keep long names readable within the existing sidebar width.
- Favorites appear only when nonempty and matching search. Favorited operators are hidden from All operators, so each operator appears once. Removing a favorite returns it to All operators.
- Right-click favorites use a compact context-menu action, without a nested push button; hover must not highlight the whole popup. Mouse clicks on switches must not add an outer focus ring. Preserve a keyboard focus indicator.
- Operator changes fade only the identity for approximately 120 ms while controls update immediately. Settings uses a stationary 180 ms crossfade. Rapid input must settle on the last selection.
- Scrollbars reserve their width, start invisible, appear on scrolling, stay visible during dragging, and fade after three idle seconds.
- Reactive radius remains a supported setting. Do not remove it as a typography cleanup.
- Nine language entries ship in the picker with painted flags: English (US), Portuguese (Brasil), French, Spanish, German, Arabic, Hebrew, Simplified Chinese, Japanese; ids 0-8 are validated in `core.cpp`, `profile_transfer.hpp`, and `phone_bridge.cpp`. The Hebrew entry is intentionally a "Free Palestine" prank (Israel flag in the picker, then a Free Palestine popup with the Palestine flag and Arabic prank strings in `src/lang_he.hpp` and the phone dictionary) and must stay that way. Translators: every language table in `src/lang_*.hpp` and every `phoneTexts` map in `assets/phone.html` must keep the full 112 / 80 keys in enum order; the `static_assert`s enforce the desktop count.
- Localized text must never widen the fixed 200 px sidebar: sticker chips elide (`StickerLabel`), setting hints word-wrap, and `tests/ui_checks.cpp` asserts the sidebar fits for every language.

## Phone interface

- Serve the self-contained phone page from the desktop app; keep it usable without external fonts, scripts, or internet access.
- The phone page reuses the desktop GoofyUI materials: cream canvas, white cards with 3 px ink outlines and hard offset shadows, tilted sticker section chips, yellow number chips, embedded Fredoka, and face toggles. It has no hover effects either.
- The phone uses the PC's LAN address, not its own localhost. Show the actual address and pairing code in desktop Settings.
- Preserve session-token authorization, pairing rate limits, host/origin checks, request-size limits, and server disable behavior.
- Validate values before applying them. Profile edits must carry explicit operator and weapon identifiers to avoid updating a different profile after concurrent selection changes.
- Save through the same persistence path as the desktop. Verify primary/secondary isolation and persistence after reopening.
- Do not add router port forwarding, public hosting, or external telemetry as part of local phone support.

## Repository and delivery

Keep source, embedded assets, tests, build/setup scripts, documentation, and third-party license files in Git. Generated executables, deployed Qt DLLs, `.build/`, personal configurations, credentials, and pairing info are excluded by `.gitignore`.

Preserve third-party attribution and licenses. Package the root launcher together with `runtime/` and license files for runnable releases. Exclude stale executable backups and local settings. Report relevant checks and any actual testing limitations; do not claim a physical iPhone or in-game test without performing it.

The local working folder keeps its historical `KingBoss2004` name because the in-use checkout cannot be renamed; the GitHub repository is `Azec-Zuckerberg/Azec`, and every application name, window, written file, package, and document uses `Azec`. Do not reintroduce the old name in code, files, or compatibility fallbacks.
