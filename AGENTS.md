# Project instructions

## Scope and architecture

KingBoss2004 is a Windows x64 C++17 / Qt 6.8.3 desktop application with an embedded local-network phone interface. Read `README.md` before changing behavior or packaging.

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
- `assets/phone.html`: self-contained mobile UI, embedded by `assets/operators.qrc`.
- `src/launcher.cpp`: root launcher; runs `runtime/KingBoss2004.exe` with the launcher directory as its working directory.
- `tests/phone_test.py`: HTTP integration checks against an isolated dry-run instance.
- `tests/core_checks.cpp` / `tests/ui_checks.cpp`: separate verification code, excluded from the release executable.

## Build and verification

Use PowerShell from the project root. Requires Visual Studio 2022 C++ tools, a Windows SDK, and Qt 6.8.3 MSVC x64. `setup-qt.ps1` installs the pinned Qt kit; `NEON_QT_ROOT` can override its location.

```powershell
.\build.cmd test
```

This builds and deploys the app, creates the root launcher and separate verification executable, and runs core plus isolated HTTP/restart checks. Build intermediates belong in `.build/`; deployed binaries belong in `runtime/`. Exclude `runtime/KingBoss2004-checks.exe` from runnable releases.

Run UI checks in an isolated working directory because the checks write fixture configurations:

```powershell
New-Item -ItemType Directory -Force .build\verification | Out-Null
Push-Location .build\verification
& '..\..\runtime\KingBoss2004.exe' --ui-self-test
Pop-Location
```

Use separate fixture directories and `QT_SCALE_FACTOR=1` / `1.5` for both display scales. Inspect the JSON report and screenshots. Match testing to the change; UI changes require visual inspection, and profile/network changes require behavior checks. Use `--dry-run` for interactive inspection and browser tests so no mouse events or beeps are generated. Do not run fixture tests from the project root or use them against the user's live configuration.

`python tests/run_checks.py --ui-only` creates fresh fixture directories and runs both scales. Build with `build.cmd test` first; the normal `--ui-self-test` command delegates to the separate verification executable.

For phone checks, start the runtime with `--dry-run --phone-info=phone-info.json` in a fresh fixture directory, then run `python tests/phone_test.py <fixture-directory>`. The info file contains temporary session credentials and must remain local. Shut down test processes started by your work after verification.

## Data and behavior contracts

- Preserve the original 3,734-byte primary configuration layout in `r6_config.bin`. Keep backward compatibility when adding features.
- Store secondary profiles, weapon hotkeys, and the phone-access preference in `kingboss_weapons.json`. Favorites use `kingboss_ui.json`.
- Each operator has a primary profile and an optional independent secondary profile. First enable copies primary settings; disabling preserves secondary edits. Character selection starts on primary.
- Default weapon keys are 1 and 2 and are configurable. Weapon shortcuts must not interrupt text/number editing or Ctrl/Alt/Windows-key shortcuts.
- Preserve fractional accumulation, timing, and SendInput behavior unless the requested change concerns them. Reset fractional movement when the active operator/weapon changes.
- Use Engine synchronization when accessing configuration from worker threads. Keep UI updates on the Qt GUI thread.
- Never replace, commit, or package the user's personal configuration files as defaults.

## UI contracts

- Retain the compact window, 200 px sidebar, custom window controls, rounded corners, the Atelier charcoal palette, the neutral chrome default accent (`#D2D2D2`), and stationary raised right panel.
- Use bundled Geist for controls and operator names. The top-left King Boss wordmark is seven lines of Atelier-cyan ASCII artwork rendered with Consolas inside the existing header.
- Header icons share 34 px hit targets and centered 20 px vector artwork. Hover changes only their ink subtly, without background tiles. Keep Save flat, without a glow, pulse, or checkmark swap; its tooltip confirms success.
- Save stays neutral gray while saved and uses the current accent while unsaved, including after a failed save. Keep unsaved data state independent of temporary feedback. Pairing QR starts obscured, reveals on explicit click/keyboard activation, and conceals on leaving Settings, hiding the window, or changing the link.
- Use 22 px outer window corners, 24 px panel corners, and soft 10-14 px button/input corners. Sliders have pill tracks and circular handles. Preserve the existing layout and hit targets.
- Operator rows are 52 px tall with 44 px icons and 14 px Geist names. Keep long names readable within the existing sidebar width.
- Favorites appear only when nonempty and matching search. Favorited operators are hidden from All operators, so each operator appears once. Removing a favorite returns it to All operators.
- Right-click favorites use a compact context-menu action, without a nested push button; hover must not highlight the whole popup. Mouse clicks on switches must not add an outer focus ring. Preserve a keyboard focus indicator.
- Operator changes fade only the identity for approximately 120 ms while controls update immediately. Settings uses a stationary 180 ms crossfade. Rapid input must settle on the last selection.
- Scrollbars reserve their width, start invisible, appear on scrolling, stay visible during dragging, and fade after three idle seconds.
- Reactive radius remains a supported setting. Do not remove it as a typography cleanup.

## Phone interface

- Serve the self-contained phone page from the desktop app; keep it usable without external fonts, scripts, or internet access.
- The phone uses the PC's LAN address, not its own localhost. Show the actual address and pairing code in desktop Settings.
- Preserve session-token authorization, pairing rate limits, host/origin checks, request-size limits, and server disable behavior.
- Validate values before applying them. Profile edits must carry explicit operator and weapon identifiers to avoid updating a different profile after concurrent selection changes.
- Save through the same persistence path as the desktop. Verify primary/secondary isolation and persistence after reopening.
- Do not add router port forwarding, public hosting, or external telemetry as part of local phone support.

## Repository and delivery

Keep source, embedded assets, tests, build/setup scripts, documentation, and third-party license files in Git. Generated executables, deployed Qt DLLs, `.build/`, personal configurations, credentials, and pairing info are excluded by `.gitignore`.

Preserve third-party attribution and licenses. Package the root launcher together with `runtime/` and license files for runnable releases. Exclude stale executable backups and local settings. Report relevant checks and any actual testing limitations; do not claim a physical iPhone or in-game test without performing it.
