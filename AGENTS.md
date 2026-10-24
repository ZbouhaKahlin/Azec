# Azec

Azec is a Windows x64 desktop application for Rainbow Six operator and weapon profiles, built with C++17 and Qt 6.8.3. It serves a companion phone interface over the local network. Desktop and phone control the same application state and save the same profiles.

Read [README.md](README.md) for setup and everyday use. This file is the shared working agreement for coding agents; `CLAUDE.md` imports it.

## Working agreements

- Understand the current source before changing behavior. Keep changes within the request, follow existing patterns, and choose the smallest implementation that handles the real constraint.
- **Do not run tests unless the user explicitly asks.** A bug report, code change, build, or release request does not by itself authorize tests. This includes unit tests, UI self-checks, browser checks, installer smoke tests, and ad-hoc scripts that exercise behavior. Source review, diff review, and checking syntax, links, or file references are allowed.
- Preserve unrelated work, including staged changes. Never reset, stash, revert, stage, or commit another task's edits. Do not stop the running app or modify personal settings to make a build succeed.
- Interception is the intentional output backend. Preserve that architecture and its explicit setup flow; do not substitute another backend or turn setup into a silent installation.
- Explain what changed and what was actually checked. A successful build does not establish runtime behavior. Do not claim physical-phone, driver, or in-game verification without performing it.
- Write for a user who is not comfortable with code. Explain changes, options, and problems in plain English, focusing on what they mean for the app. Avoid jargon, code snippets, file paths, and line numbers unless requested or needed to identify a problem. Speak as a peer, keep routine updates short, and report incomplete work directly.
- Address the user as “you”; do not use their personal name. Do not create a pull request unless asked.

## Build and inspect

Use PowerShell from the project root. The toolchain is Visual Studio 2022 C++ tools, a Windows SDK, Python, and Qt **6.8.3 MSVC x64**.

```powershell
.\setup-qt.ps1
.\build.cmd
```

`setup-qt.ps1` installs the pinned Qt kit; `NEON_QT_ROOT` can select another compatible installation. The project uses qmake through `Azec.pro`. Build intermediates belong in `.build/`, deployed files in `runtime/`, and distributable packages in `dist/`.

- `build.cmd` builds the application without running tests.
- `build.cmd checks` also builds the separate verification executable without running it.
- `share.cmd` builds the single-file installer without tests.
- `share.cmd quick` repackages the existing share build without recompiling.
- `share.cmd publish` builds and publishes the EXE, then verifies the published artifact. See [Releasing](#releasing).

### When verification is requested

Read [docs/testing.md](docs/testing.md) before choosing a command. Its command reference does not grant permission to run tests; the explicit-request rule above takes precedence. `build.cmd test`, `build.cmd test --suite`, and the `test` argument to `share.cmd` execute tests. `AZEC_SKIP_TESTS=1` overrides installer-test requests. Keep CI testing manual and its broader coverage separately opt-in.

Choose checks relevant to the request. Do not expand into scale/language matrices, real-time expiry, or extra suites without authorization. Stop after a relevant check passes; repeat it only after a relevant fix or new evidence.

Desktop-backed checks must use the existing `tests/hidden_process.py` isolation on a noninteractive Windows station. Never open visible test windows, steal focus, move the cursor, or play sounds. If isolation fails, stop; there is no visible fallback. Hiding a console alone does not isolate a GUI.

Use a fresh fixture under `.build/verification`, never the project root or personal configuration. Use `--dry-run` to suppress mouse output and sounds. Installer fixtures also require `AZEC_INSTALL_TEST_ROOT` to redirect Programs, data, and shortcut paths. Stop the complete process trees started by your work.

An explicit screenshot request authorizes the requested captures, not a test suite. Use isolated dry-run data and keep pairing codes, token-bearing links, and revealed QR images out of shared captures. Browser emulation is not a physical-phone test.

## Where code lives

- **Startup and installation:** `src/app.cpp`, `src/startup.hpp`, `src/app_paths.hpp`, and `src/launcher.cpp` own startup, single-instance activation, recovery, storage locations, and the embedded installer. `src/update_check.hpp` owns update handling.
- **Desktop:** `src/main_window.hpp` owns bindings, settings, tray actions, and dialogs. `src/desktop_layout.hpp` owns the single desktop layout. `src/ui_widgets.hpp`, `src/app_style.hpp`, and `src/theme.hpp` own controls and presentation.
- **Profiles and output:** `src/core.hpp` / `src/core.cpp` own binary compatibility and movement arithmetic. `src/engine.hpp` / `src/engine.cpp` synchronize configuration and run input workers. `src/profile_store.hpp`, `src/ui_preferences.hpp`, and `src/profile_transfer.*` own JSON preferences and backups.
- **Driver:** `src/driver_input.hpp` dynamically loads Interception. `src/driver_setup.hpp` and `src/driver_download.hpp` own download, verification, and file placement.
- **Phone:** `src/phone_bridge.cpp` maps requests to application state. `src/phone_server.hpp` validates and serves requests; `src/lan_address.hpp` selects the PC address. `assets/phone.html` is the self-contained client, embedded through `assets/operators.qrc`.
- **Mouse setup:** `src/mouse_setup.cpp` and `src/sensitivity.hpp` own game-settings discovery and sensitivity rescaling.
- **Language and assets:** `src/strings.hpp`, `src/lang_*.hpp`, and `src/flags.hpp` own localization. Fonts, icons, flags, and phone assets are bundled; notices live in `licenses/`.
- **Verification:** `tests/` contains separate checks and isolation helpers. Verification executables and fixture DLLs never belong in a release.

## Profiles and persistence

Compatibility is a product requirement. Preserve the existing **3,734-byte** `r6_config.bin` layout and the meaning of every saved operator ID.

- There are 78 operators. The binary holds the original 71 records; extension operators (Noor through Denari, IDs 71–77) use `primaryProfiles` in `azec_weapons.json`. Selection also persists in that JSON file; the binary selection field accepts only original-roster indexes. Append roster additions; never reorder existing records.
- `azec_weapons.json` also holds secondary profiles, per-profile rapid-fire flags, weapon bindings, phone access, game-focus/aiming preferences, and mouse setup. `azec_ui.json` holds favorites, `colorTheme`, and `sidebarCollapsed`.
- UI-preference writes must preserve other preferences. Ignore the obsolete `interface` key and remove it only on a successful atomic save. Normalize legacy binary appearance values on load and the next save; named themes do not change profile values.
- Keep save staging, atomic replacement, and rollback across configuration files. A failure must retain unsaved edits and show a persistent error. Never replace, commit, or package personal configuration as defaults.
- A secondary profile copies primary settings only on first enable. Disabling preserves its edits. Selecting an operator starts on primary and persists only the selection, without marking profiles dirty or saving unrelated edits.
- Accept backups containing 71–78 operators, matching records by name and preserving operators absent from an older backup. Imported profiles and UI preferences remain pending until Save; Discard retains the previously saved files.
- Mouse DPI is reference metadata. Rescale recoil using old/new game sensitivity and multiplier per axis, preview the change, and reject the entire operation if any affected profile exceeds its range. Backups include numeric setup values, not the local INI path. Dry-run instances skip automatic game-settings discovery.

## Output and shortcuts

Recoil is a live output switch and starts off on launch. Rapid fire and shot delay belong to the active weapon profile; primary and secondary are independent. Keep the header, tray, desktop controls, and phone Output card synchronized. Do not restore the removed Master enable gate.

Preserve migration from legacy `rapidActive` onto every primary and secondary profile. New saves write `perProfileRapid` and per-profile flags, not a shared rapid-fire switch.

- Interception sends relative mouse movement and left-button strokes. Load `interception.dll` at runtime; do not link or bundle the driver. Keep its files in the persistent data folder, preserve migration from adjacent DLLs, and verify the pinned setup archive's SHA-256 before extraction.
- **Only in Rainbow Six** is on by default and gates output and application shortcuts by the foreground game. Panic is also ignored while Azec is active. Preserve **Only while aiming**. These are focus/interaction rules; do not make anti-cheat guarantees.
- Preserve fractional accumulation, recoil/fire timing, and stop behavior unless the request concerns them. Reset fractional movement when the active operator or weapon changes. Access worker configuration through Engine synchronization; keep widgets on the Qt GUI thread.
- Default weapon bindings are `1` and `2`. Show/hide, recoil-toggle, and weapon bindings accept the user's chosen keys, including arrows, numpad, mouse side buttons, modifiers, and Windows keys. Reject duplicates with the visible “key already used” message. Panic keeps its stricter safe-key filter.
- Weapon shortcuts must not interrupt text/number editing or Ctrl/Alt/Windows shortcuts. Focused lists keep arrow-key navigation; recoil fine-tuning applies only when no list, text/number field, or slider owns the keyboard.

## Desktop and phone design

The design reference is [GoofyUI for C++ / Qt](https://github.com/Azec-Zuckerberg/goofy-ui-cpp). Use that repository for design guidance instead of the former Luau library. Azec currently owns its Qt controls locally; a documentation or styling request does not authorize replacing them or changing the build system.

Keep one desktop layout in `src/desktop_layout.hpp` and the corresponding phone presentation. Themes change colors and materials, not behavior or layout. The eleven palettes in `src/theme.hpp` are authoritative, including Cocoa & mint; **Charcoal & blush** is the fresh-install default. Appearance shows all palettes as previews in a three-column grid and saves choices immediately. Do not add an interface selector, free accent picker, or radius slider.

### Preserve the interaction model

- Use flat color, chunky ink outlines, hard offset shadows, rounded panels, sticker headings, yellow numeric chips, and face toggles. No gradients, glow, or blurred shadows. Keep bundled Geist for body/controls, Fredoka for display headings, and Press Start 2P for the outlined Azec wordmark.
- Retain the compact window, custom window controls, raised right panel, and seamless header. The sidebar expands to 200 px and collapses to 76 px; its Collapse control is independent of the theme. Search stays at the bottom and compact search expands and focuses it. Favorites appear once, only when nonempty and matching search, with a compact star shortcut in the collapsed sidebar.
- Preserve existing hit targets and legibility: 52 px operator rows with 44 px icons, 34 px header targets with 20 px artwork, 24 px window/panel corners, and at least 20 px slider height. Long names and translations must fit without widening the sidebar.
- Controls stay flat on hover; only context-menu items highlight. Keyboard focus uses an accent outline. Mouse clicks on switches must not add that keyboard ring. Keep favorite actions in a compact context menu without a nested button.
- Save stays flat with unchanged artwork: neutral ink when saved, accent when dirty, with readable foreground text. Success uses the tooltip; failures use a persistent banner. Temporary feedback must not clear the dirty state. Closing unsaved edits shows Save/Discard/Cancel; another close re-surfaces the prompt, and Cancel keeps the app running. Keep the close-confirmation and Quick Guide dialogs visually consistent with the hotkey dialog.
- Generate feedback sounds inside the app and honor the sound preference. The unsaved-close cue is a soft descending two-note sound; do not use a Windows error beep or console bell.
- Operator changes fade only the identity for about 120 ms while controls update immediately. Settings uses a stationary 180 ms crossfade. Rapid changes settle on the last selection. Avoid continuous repainting and unbounded image caches.
- Reserve scrollbar lanes. Show bars during scrolling/dragging and fade them after three idle seconds. Keep smooth wheel notches, native precision-touchpad deltas, and shared edge fades. An unfocused slider, number field, or dropdown forwards wheel scrolling to its page.

Settings categories remain General, Appearance, Shortcuts, Phone control, Profile backup, and Mouse setup.

### Keep both screens consistent

A change to profiles, output switches, themes, language, or saving needs a decision for the desktop, tray where applicable, and phone. Keep shared behavior in the existing Engine/persistence paths rather than creating a separate phone implementation.

- Serve the phone page locally with embedded assets and no external fonts, scripts, or internet requirement. Keep its palettes sourced from desktop theme tokens. A theme change on either screen saves the same preference and updates both without saving profile edits.
- The first card is **Output**, with live **Recoil active** and active-profile **Rapid fire active** switches. Phone profile edits carry explicit operator and weapon IDs so concurrent selection cannot redirect a write. Validate values before applying them and save through the desktop path.
- Use the PC's actual LAN address, never phone localhost. Preserve the selectable six-digit code, Copy code, Copy link, New code, and startup-error Retry action. New code rotates both code and token and disconnects existing phones.
- QR starts concealed and reveals only on explicit click/keyboard activation. Conceal it when leaving Phone control, hiding the window, or changing the link. Palette changes rebuild its textures without changing the encoded link. Keep the complete unbranded pattern, dark-on-light contrast, and four-module white quiet zone.
- Preserve session-token authorization, pairing rate limits, host/origin checks, request-size limits, and server-disable behavior. Pause polling while the phone page is hidden. Do not add public hosting, router forwarding, or telemetry to local phone support.
- Pairing codes, token-bearing links, and revealed QR captures are credentials. Keep them local and out of logs, Git, documentation images, and release assets.

## Localization

Fresh configurations and the unpaired phone page start in English; pairing follows the desktop language. Preserve language IDs 0–8, including Portuguese `0` and English `1`.

The nine picker entries are English, Portuguese, French, Spanish, German, Arabic, Hebrew, Simplified Chinese, and Japanese. The Hebrew entry intentionally opens the Free Palestine popup and uses the existing Arabic prank strings and flags; preserve that behavior unless asked to change it.

Every `src/lang_*.hpp` table must cover `UiTextCount` in enum order. Keep the compile-time checks. Every phone dictionary must cover the same phone keys, including Output controls. Preserve RTL handling, bundled flags, elided sidebar stickers, and wrapped settings hints.

## Installation and updates

The embedded `dist/Azec.exe` installs per user under `%LOCALAPPDATA%\Programs\Azec`, creates a Start menu shortcut, and stores profiles, logs, and the single-instance lock separately under `%LOCALAPPDATA%\Azec`. Development/portable launchers retain adjacent-runtime/current-folder behavior.

Preserve settings across updates. Copy portable settings only into an empty installed store and retain their originals. Prepare a versioned runtime before switching the launcher; never downgrade a newer installation. Verify an update's SHA-256 before executing it.

Keep sources, embedded assets, build scripts, documentation, tests, and third-party notices in Git. Keep generated executables, deployed Qt DLLs, `.build/`, personal settings, logs, credentials, pairing captures, and stale backups out of Git. Packages must also exclude personal data, stale executable backups, and all verification executables and fixture DLLs. `package-release.ps1` remains the optional folder-style portable ZIP packager. Preserve attribution and licenses.

## Releasing

“Build and push,” “ship a release,” “publish an update,” and “release a new version” authorize both the source commit/push and public EXE publication. Complete both without asking again. Honor restrictions such as build-only, source-only, no push, or no publishing. An ordinary code or documentation edit is not a release request.

1. Review the working tree and include only this task's changes. Confirm the source destination is private `Azec-Zuckerberg/Azec` and the binary destination is public `Azec-Zuckerberg/Azec-releases`. Do not stop the user's app or alter personal settings.
2. Follow the test rule: releases do not authorize tests. Resolve failures from any separately authorized checks. Artifact/version/hash verification below is required delivery verification and does not run application behavior tests.
3. Run `.\share.cmd publish`. It stamps `src/version.hpp`, rebuilds, packages, publishes **only `Azec.exe`**, and invokes `verify-release.ps1`. Do not substitute `quick`, a local build, or a source push. Add `test` only when the isolated installer smoke test was explicitly requested.
4. Confirm the published tag matches `src/version.hpp`. Download that exact release into a fresh `.build/verification` directory. Its SHA-256 must match both GitHub's asset `digest` and local `dist/Azec.exe`; a missing digest or mismatch is a failure. Confirm the stable latest-download URL serves the new release anonymously.
5. Commit and push this task's source changes, including the generated version, to the intended branch (normally `main`). Preserve unrelated/staged work; never blanket-stage or force-push. Verify the remote contains the source commit.
6. Report the version, source commit, checks performed/skipped, and [stable EXE download](https://github.com/Azec-Zuckerberg/Azec-releases/releases/latest/download/Azec.exe). Existing users update through **Settings → General → Updates → Check for updates**, with AppData profiles preserved.

Only the clean EXE belongs in uploaded public assets: no checksum file, private source, profiles, logs, credentials, or verifier. The updater uses GitHub's asset digest. GitHub's automatic source archives cannot be removed through the release-assets API; the separate public release repository must contain only its release README/scaffolding. If publication, digest verification, authentication, or source push fails, report the exact incomplete step rather than calling the release complete.

## Documentation and naming

Keep the README focused on installation and user tasks. Put agent working rules here, detailed application behavior in [docs/reference.md](docs/reference.md), and diagnostic commands in [docs/testing.md](docs/testing.md). Update affected guidance rather than appending a second explanation. Do not add documentation for every small code change or commit temporary plans and research notes.

Use **Azec** for the app, windows, packages, and documentation. The checkout's historical `KingBoss2004` folder name remains only because the in-use checkout cannot be renamed. Do not introduce that name into application files or compatibility fallbacks.
