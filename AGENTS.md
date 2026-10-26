# Azec

Azec is a Windows x64 desktop application for Rainbow Six operator and weapon profiles, built with C++17 and Qt 6.8.3. It drives mouse output through Interception and serves a companion phone interface over the local network. Desktop, tray, and phone control the same application state and save the same profiles.

This file is the shared working agreement for coding agents; `CLAUDE.md` imports it. Read [README.md](README.md) for setup and everyday use.

## Working agreement

- Understand the current source before changing behavior. Follow existing patterns and choose the smallest change that handles the real constraint. Do not preserve unnecessary complexity or add machinery for hypothetical needs.
- Preserve unrelated work, including staged changes. Never reset, stash, revert, stage, or commit another task's edits. Never blanket-stage or force-push.
- The person directing development does not write code. Explain changes, problems, and actual checks in plain English. Avoid jargon, code snippets, file paths, and line numbers unless requested or needed to identify a problem. Address them as "you"; never use their personal name. Keep routine updates short and report incomplete work directly.
- Requirements below protect saved data, the running app, and delivery. Design and implementation preferences guide choices within those requirements. An explicit user instruction changing a project rule takes precedence over this file, including a request to replace a protected implementation choice; carry out already authorized work without asking again. Ask only about unresolved scope or consequences beyond that authorization.
- Requirements describe intended behavior, not proof that the current source meets them. Report implementation discrepancies with evidence. A documentation audit may correct descriptive text, but must not weaken protective requirements or silently change application behavior to reconcile a mismatch.
- When delegating, pass the task scope, relevant project rules, execution scope, and known unrelated changes. Delegation does not expand permission to publish or touch personal data. Require evidence and check findings before acting on them; the delegating agent owns the final result.
- Do not create a pull request unless asked.

## The ways to hurt yourself

1. **Running behavior in someone else's session.** Run tests, checks, builds, and the app whenever a check answers a real question; no separate permission is needed. A build or a review is not enough on its own to claim behavior, so actually run what the claim depends on. Keep desktop-backed runs isolated and invisible as described in [Verification](#verification), and stop every process you start.
2. **Stopping the running app.** Azec may be running on this machine with the driver loaded. Never kill processes by name, window title, or path match. Stop only processes and process trees started by your work, using captured process IDs. Do not stop the user's app or alter personal settings to make a build succeed.
3. **Writing to live data.** `%LOCALAPPDATA%\Azec` contains real profiles, logs, and the lock. Never use it as a test fixture or clean it up. Portable configuration beside a build or in its working folder may also be live; do not treat `runtime/` as disposable. Never replace, commit, or package personal configuration as defaults.
4. **Leaking credentials.** Pairing codes, token-bearing links, and revealed QR images are credentials. Keep them local and out of logs, Git, documentation images, shared captures, and release assets.

## A small glossary

- **Operator:** a Rainbow Six operator. There are 78; existing IDs 0–77 are permanent.
- **Profile:** one operator's recoil and weapon settings. Primary and secondary are independent; the named library holds saved sets.
- **Binary:** `r6_config.bin`, fixed at 3,734 bytes, holding the original 71 operator records, language, and other compatible settings.
- **Sidecar:** `azec_weapons.json`, holding extension operators, selection, secondary profiles, per-profile rapid-fire flags, weapon bindings, phone access, focus/aiming preferences, sensitivity, and the named library.
- **UI preferences:** `azec_ui.json`, holding favorites and the local `theme` choice. Language is stored in the binary.
- **Engine:** the shared configuration and worker synchronization layer.
- **Driver:** Interception; `interception.dll` is loaded at runtime, never linked or bundled.
- **Phone:** the self-contained LAN page, paired by a six-digit code and session token.
- **Surface:** the desktop window, tray, or phone.

## Hit every surface

For a relevant behavior change, review these areas before calling work done. Briefly report the applicable areas and what was actually checked.

- **Surfaces:** changes to profiles, output switches, themes, language, or saving need a decision for the desktop, tray where applicable, and phone. Keep shared behavior in the existing Engine and persistence paths.
- **Both weapons:** primary and secondary are independent. Rapid fire and shot delay belong to the active weapon profile.
- **Persistence:** profile edits remain pending until Save. Discard retains saved files. Failed saves retain unsaved edits and show a persistent error. Preserve existing immediate-save paths for selection, shortcuts, and local UI preferences without saving unrelated pending edits. Phone edits use the same pending state and save path.
- **Old data:** backups with 71–78 operators import by name, preserve operators absent from an older backup, and remain pending until Save.
- **Languages:** every desktop language table covers `UiTextCount` in enum order, and every phone dictionary covers the same phone keys. Preserve compile-time checks and RTL behavior.
- **Reverse states:** QR reveal needs conceal; New code disconnects paired phones; disabling a secondary preserves its edits; another close re-surfaces the unsaved-close prompt, and Cancel keeps the app running.
- **Shortcuts:** preserve accepted keys, duplicate rejection, editing and navigation behavior, and panic's stricter safe-key filter.
- **Docs:** update existing guidance when the change makes it inaccurate. Do not append a second explanation.

## Build

Use PowerShell from the project root. The toolchain is Visual Studio 2022 C++ tools, a Windows SDK, Python, and Qt **6.8.3 MSVC x64**.

- `.\setup-qt.ps1` installs the pinned Qt kit; `NEON_QT_ROOT` selects another compatible installation.
- `.\build.cmd` builds the application without running tests.
- `.\build.cmd checks` also builds the separate verification executable without running it.
- `.\share.cmd` builds the single-file installer without tests; `quick` reuses the compiled application, rewrites `src/version.hpp` to that build's version, and rebuilds the installer wrapper.
- `.\share.cmd publish` builds, packages, publishes, and verifies the EXE; see [Releasing](#releasing).

Build intermediates belong in `.build/`, deployed files in `runtime/`, and distributable packages in `dist/`. The project uses qmake through `Azec.pro`.

If deployment into `runtime/` fails because files are in use, report the incomplete deployment. Do not stop the user's app to finish the build.

## Verification

Read [docs/testing.md](docs/testing.md) before choosing a verification command.

- `.\build.cmd test` runs the default suite and `.\build.cmd test --suite` runs the complete one. Match the scope to the question; a focused check is usually enough.
- `build.cmd test`, `build.cmd test --suite`, and the `test` argument to `share.cmd` execute tests. `AZEC_SKIP_TESTS=1` skips only the `share.cmd` installer smoke test; it does not suppress `build.cmd test`. Keep CI testing manual.
- Desktop-backed checks must use `tests/hidden_process.py` isolation on a noninteractive Windows station. Never open visible test windows, steal focus, move the cursor, or play sounds. Hiding a console alone does not isolate a GUI. If isolation fails, stop; there is no visible fallback.
- Use a fresh fixture under `.build/verification`, never the project root or personal configuration. Use `--dry-run` to suppress mouse output and sounds. Installed fixture executables honor `AZEC_INSTALL_TEST_ROOT` only together with `--dry-run`; without that flag the environment variable does not redirect Programs, data, or shortcuts. The installer Python runner supplies both itself. Stop the complete process trees started by your work.
- Ordinary dry-run fixtures skip startup update checks. Use the focused startup, timing, or startup-update checks in [docs/testing.md](docs/testing.md) for that behavior; update fixtures use a local server and an install callback, without executing a downloaded installer. Rebuild after C++ or embedded-asset changes before relying on executable checks.
- Choose checks relevant to the request. Do not expand into scale/language matrices, real-time expiry, or extra suites unless that coverage is the question being answered. Stop after the relevant check passes; repeat only after a relevant fix or new evidence.
- A screenshot request asks for captures, not a test suite. Use isolated dry-run data and exclude pairing credentials. Browser emulation is not a physical-phone test.
- Explain what was actually checked. A successful build does not establish runtime behavior. Never claim physical-phone, driver, or in-game verification without performing it.

## Releasing

"Build and push," "ship a release," "publish an update," and "release a new version" authorize the source commit/push and publication of the same EXE release to both the private and public repositories. Complete all three without asking again. Honor restrictions such as build-only, source-only, no push, or no publishing. An ordinary edit is not a release request. Publishing does not require tests; the artifact, version, and hash verification below is required delivery verification.

Source-only delivery does not require installer version stamping or public-asset verification. If a release stops partway through, inspect the existing tag, local artifact, verification results, and source commit before resuming. After a successful upload, retry `verify-release.ps1` with `-Version` set to that release's numeric version; do not rerun `share.cmd publish`, which stamps a new version. To resume an interrupted download, also pass `-ResumeDirectory` with the verification folder printed by that run; completed downloads still require the full published hash. If no release was created, fix the failure and restart publication normally. Do not create a new release or replace a published asset just to bypass a verification failure.

1. Review the full working tree, not only staged files: packaging compiles the working files. Keep unrelated application source or embedded-asset edits out of the EXE, using a separate checkout with the intended task changes when necessary. Confirm the source destination is private `Azec-Zuckerberg/Azec` and the binary destinations are public `Azec-Zuckerberg/Azec-releases` and private `Azec-Zuckerberg/Azec`. Preserve unrelated and staged work.
2. Resolve failures from your checks. Do not stop the user's app or alter personal settings.
3. Run `.\share.cmd publish`. It stamps `src/version.hpp`, rebuilds, packages, publishes **only `Azec.exe`**, and invokes `verify-release.ps1`. Do not substitute `quick`, a local build, or a source push. Add `test` to include the isolated installer smoke test.
4. Confirm the published tag matches `src/version.hpp`. Download that exact release into a fresh `.build/verification` directory. Its SHA-256 must match both GitHub's asset `digest` and local `dist/Azec.exe`; a missing digest or mismatch is a failure. Confirm the stable latest-download URL serves the new release anonymously.
5. Commit and push this task's source changes, including the generated version, to the intended branch (normally `main`). Never blanket-stage or force-push. Verify the remote contains the source commit.
6. Publish the same `dist/Azec.exe` under the same version tag in private `Azec-Zuckerberg/Azec`, targeting the pushed source commit. Use `gh release create` with explicit `--repo`, `--target`, title, and notes. If that tag already exists, inspect it before resuming; do not replace assets to bypass a verification failure. Confirm its sole asset is `Azec.exe`, its GitHub SHA-256 digest matches the local EXE and public asset, and an authenticated download has that same hash. Confirm its tag resolves to the pushed source commit.
7. Report the version, source commit, checks performed/skipped, and [stable EXE download](https://github.com/Azec-Zuckerberg/Azec-releases/releases/latest/download/Azec.exe). Existing users update through **Settings → General → Updates → Check for updates**, with AppData profiles preserved.

Only the clean EXE belongs in uploaded public assets: no checksum file, private source, profiles, logs, credentials, or verifier. The updater uses GitHub's asset digest. GitHub's automatic source archives cannot be removed through the release-assets API; the separate public release repository must contain only its release README/scaffolding. Publication makes the EXE public before verification runs; a verification failure leaves it available. If publication to either repository, digest verification, authentication, or source push fails, report the exact incomplete step and public state instead of calling the release complete.

## How it works

The Engine owns shared configuration and synchronizes input workers. Desktop widgets edit pending configuration on the Qt GUI thread; the phone bridge maps validated requests onto the same application state and persistence paths. Workers send relative mouse movement and left-button strokes through the verified Interception DLL, subject to focus and aiming gates.

Normal Save writes the binary and sidecar using staging, atomic replacement, and rollback. When an imported backup has pending UI preferences, those join the save transaction. Local theme and ordinary favorites/sidebar changes use their existing UI-preference save paths. Keep these distinctions: do not make a profile save overwrite unrelated preferences or commit unrelated pending edits.

## Where code lives

- **Startup and installation:** `src/app.cpp`, `src/startup.hpp`, `src/app_paths.hpp`, `src/launcher.cpp` (startup, single-instance activation, recovery, storage, embedded installer); `src/startup_update.hpp`, `src/update_check.hpp` (startup gate and verified updates); `src/startup_view.hpp` (loading and desktop reveal).
- **Desktop:** `src/main_window.hpp` (bindings, settings, tray, dialogs); `src/desktop_layout.hpp` (the single desktop layout); `src/ui_widgets.hpp`, `src/mono_controls.hpp`, `src/interaction_icons.hpp`, `src/app_style.hpp`, `src/theme.hpp` (controls, motion, presentation, design tokens); `src/sound.hpp` (generated feedback cues).
- **Profiles and output:** `src/core.hpp` / `src/core.cpp` (binary compatibility, movement arithmetic); `src/engine.hpp` / `src/engine.cpp` (configuration synchronization, input workers); `src/profile_store.hpp`, `src/ui_preferences.hpp`, `src/profile_transfer.*` (JSON preferences, backups).
- **Driver:** `src/driver_input.hpp` (runtime loading); `src/driver_setup.hpp`, `src/driver_download.hpp`, `src/driver_trust.hpp` (setup, pinned hashes, verification, file placement).
- **Phone:** `src/phone_bridge.cpp` (requests to application state), `src/phone_server.hpp` (validation, serving), `src/lan_address.hpp` (PC address); `assets/phone.html` (self-contained client embedded through `assets/operators.qrc`).
- **Stored sensitivity and sets:** `src/sensitivity.hpp` and `src/profile_sets.hpp` (sensitivity metadata, conversion arithmetic, named profile compatibility).
- **Language and assets:** `src/strings.hpp`, `src/lang_*.hpp`, `src/flags.hpp`. Icons, flags, and phone assets are bundled; design and sound reference notices live in `licenses/`.
- **Verification:** `tests/`. Verification executables and fixture DLLs never belong in a release.

## Profiles and persistence

- Preserve the **3,734-byte** binary and the original 71 records in fixed order. Extension operators (Noor through Denari, IDs 71–77) use the sidecar's `primaryProfiles`. Selection also persists in the sidecar; the binary selection field accepts only original-roster indexes. Append roster additions; never reorder existing records or change the meaning of a saved operator ID.
- Preserve save staging, atomic replacement, and rollback across configuration files. A failed save retains unsaved edits and shows a persistent error.
- On startup, preserve rejected binary or JSON settings in unique `.corrupt-*` files before allowing replacement, and retain the recovery notice. If a file cannot be read or preserved, block configuration saving rather than overwrite it with defaults. Keep the single-instance lock scoped to the configuration directory and never reclaim a live owner's lock.
- UI-preference writes preserve other preferences. Ignore obsolete `interface`, `colorTheme`, and `sidebarCollapsed` keys and remove them only on a successful atomic save. Normalize legacy binary appearance values on load and the next save.
- A secondary profile copies primary settings only on first enable. Disabling preserves its edits. Changing operators starts on primary and persists only selection, without marking profiles dirty or saving unrelated edits.
- Accept backups containing 71–78 operators, matching by name and preserving operators absent from older backups. Imported profiles and UI preferences remain pending until Save; Discard retains previously saved files.
- Named sets live in the sidecar and backups. The Converter & profiles management page and unused handlers are removed; preserve existing library data, active-set edits, and backup compatibility. Phone edits operate on the same active set.
- Mouse DPI stays reference metadata in stored sensitivity values. Retain conversion arithmetic for compatibility; there is no sensitivity-converter UI or automatic game-settings discovery. Backups contain numeric setup values, not the local INI path.
- Migrate legacy `rapidActive` onto every primary and secondary profile. New saves write `perProfileRapid` and per-profile flags, not a shared rapid-fire switch.

## Output and shortcuts

- Recoil is a live output switch and starts off on launch. Rapid fire and shot delay belong to each weapon profile independently. Keep the header, tray, desktop controls, and phone Output card synchronized. Do not restore the removed Master enable gate.
- Interception is the intentional output backend. Load `interception.dll` at runtime; do not link or bundle the driver, substitute another backend, or turn explicit setup into silent installation.
- Keep driver files in the persistent data folder and preserve migration from adjacent DLLs. Verify the pinned setup archive's SHA-256 before extraction and every DLL against the pinned x64 hash before copying or loading. Guard the file and resolved parent directories against replacement while loading. Only offer the installer command for the pinned installer.
- **Only in Rainbow Six** is on by default and gates output and application shortcuts by the foreground game. Panic is ignored while Azec is active. Preserve **Only while aiming**. These are focus/interaction rules; do not make anti-cheat guarantees.
- Preserve fractional accumulation, recoil/fire timing, and stop behavior unless the request concerns them. Reset fractional movement when the active operator or weapon changes. Configure workers through Engine synchronization and keep widgets on the Qt GUI thread.
- Default weapon bindings are `1` and `2`. Show/hide, recoil-toggle, and weapon bindings accept the chosen keys, including arrows, numpad, mouse side buttons, modifiers, and Windows keys. Reject duplicates with the visible "key already used" message. Panic keeps its stricter safe-key filter.
- Weapon shortcuts must not interrupt text/number editing or Ctrl/Alt/Windows shortcuts. Focused lists keep arrow-key navigation; recoil fine-tuning applies only when no list, text/number field, or slider owns the keyboard.

## Design

The reference is [monocode](https://github.com/hardbeat920/monocode) (MIT; notice in `licenses/monocode/`), rebuilt natively in Qt. Keep one desktop layout in `src/desktop_layout.hpp` and the corresponding phone presentation. Azec owns its controls locally; a documentation or styling request does not authorize replacing them or changing the build system.

Consult [Appearance and settings](docs/reference.md#appearance-and-settings) and the overview in that document before changing presentation. They retain the detailed glass, motion, scrollbar, sizing, and sidebar requirements. Color tokens in `src/theme.hpp` are authoritative; the phone follows them through `appearance.colors`.

- General offers only Dark and Light; default Dark. No palette picker, interface selector, free accent picker, radius slider, glass preference, or Appearance category. Settings categories remain General, Shortcuts, Phone control, and Profile backup. Do not restore the Converter tab.
- Use translucent content fills over window glass, 1 px hairline separators, borderless action buttons, a single blue accent, system fonts (Segoe UI with Consolas numerics), 6–10 px control corners, and soft shadows only on floating surfaces. Recoil and Rapid fire in the header are flat icon-and-label controls with no background boxes, including on hover or press; retain their hit targets and keyboard focus outline. No gradients, glow, chunky outlines, hard offset shadows, sticker headings, face toggles, or bundled display fonts.
- Theme changes save atomically in UI preferences, preserve other preferences and pending profile edits, and leave the prior theme selected with a persistent error if saving fails. Preserve native glass in both themes and the theme crossfade. On Windows 10, switch to lighter blur immediately at move/resize start and restore acrylic at the end, cancellation, or hide; do not reintroduce a cover or fade, or change the material on every cursor move. Reapply the rounded mask after material changes.
- Keep the compact window, custom window controls, raised right panel, seamless header, existing hit targets, and legibility. Long names and translations must fit without widening the sidebar.
- Keyboard focus uses an accent outline. Mouse clicks on switches must not add that ring.
- Save uses the approved A arrow-and-tray SVG, staying flat and transparent: neutral ink when saved, accent when dirty. Only a successful save triggers the arrow landing and drawn-check confirmation, then returns to neutral; new edits cancel that confirmation. Keep the tooltip and persistent failure banner. Temporary feedback never clears dirty state or conceals a failed save. Preserve Save/Discard/Cancel and the consistent hotkey, close-confirmation, and Quick Guide dialogs.
- Generate feedback sounds inside the app as ported cuelume recipes and honor the sound preference: mechanical toggle cues, scan cues for copy actions, success cues for explicit saves, and calm refusal cues for leaving with unsaved changes. Never use a Windows error beep or console bell.
- Avoid continuous repainting and unbounded image caches. Preserve shared motion timing and rapid changes settling on the last selection.

### Azec UX preferences

- Aim for clean, Apple-like polish through precise button design, consistent SVG icons, immediate click feedback, and subtle, satisfying motion. Keep Azec's own identity, compact layout, and Windows presentation; refine the existing controls in place.
- The approved direction includes animated Settings sliders, Copy icons that draw a check while their labels stay still, one traveling weapon-tab underline, a switch thumb that widens under pressure, and an always-expanded sidebar. Carry matching details to the phone where applicable.
- Motion should explain the action and follow its real result. Keep it brief, smooth, reversible where appropriate, and confined to the control; preserve hit targets and scroll position. Settle on the latest state and stop animating at rest. Avoid decorative loops, bounce, glow, and extra containers.
- Startup checking, loading, expansion, and content reveal share one native window, with a continuous wordmark and progress bar. The three loading subjects are a visual introduction, not measured work. Release temporary snapshots after the reveal; hiding settles the introduction without replay on reopen. Honor Windows client-area animation settings without skipping the update check. Keep recovery notices available after the reveal; detailed timings belong in [docs/reference.md](docs/reference.md#overview).
- For a new visual direction, show separate before-and-after variations with screenshots and replayable animations before changing the app. Once a variation and its accompanying details are approved, implement the full approved set without asking for the same approval again.

## Phone

- Serve the page locally and self-contained: no external fonts, scripts, or internet requirement; system fonts only.
- The first card is **Output**, with the live **Recoil active** switch. The active-profile **Rapid fire active** switch sits above the phone’s **Rapid fire** heading. Bind queued profile edits to the captured profile-set identity and explicit operator and weapon IDs; reject stale-set requests so concurrent selection cannot redirect a write. Validate values before applying them and save through the desktop path.
- Use the PC's actual LAN address, never phone localhost. Preserve the selectable six-digit code, Copy code, Copy link, New code, and startup-error Retry action. New code rotates both code and token and disconnects existing phones.
- Preserve six-digit entry and the phone's QR-photo pairing fallback on LAN HTTP. Decode photos locally with bundled jsQR; never upload them or require an external service. Accept only a token link for the current page's origin and path, retaining link and token validation and readable retry errors.
- QR starts concealed and reveals only on explicit click/keyboard activation. Conceal it when leaving Phone control, hiding the window, or changing the link. Theme changes preserve the encoded link and dark-on-white contrast. Keep the complete unbranded pattern and four-module white quiet zone.
- Preserve session-token authorization, pairing rate limits, host/origin checks, request-size limits, and server-disable behavior. Pause polling while the page is hidden. Ignore responses from obsolete sessions, including authorization failures, so they cannot restore old controls or clear a newer session. Do not add public hosting, router forwarding, or telemetry.

## Language

Fresh configurations and the unpaired phone page start in English; pairing follows the desktop language. Preserve IDs 0–8, including Portuguese `0` and English `1`.

The nine picker entries are English, Portuguese, French, Spanish, German, Arabic, Hebrew, Simplified Chinese, and Japanese. Every `src/lang_*.hpp` table covers `UiTextCount` in enum order with compile-time checks; every phone dictionary covers the same phone keys, including Output controls. Preserve RTL handling, bundled flags, elided operator names, and wrapped settings hints.

The Hebrew entry intentionally opens the Free Palestine popup and uses the existing Arabic prank strings and flags; preserve that behavior unless asked to change it.

## Installing and updating

- The embedded `dist/Azec.exe` installs per user under `%LOCALAPPDATA%\Programs\Azec`, creates a Start menu shortcut, and stores profiles, logs, and the single-instance lock separately under `%LOCALAPPDATA%\Azec`. Development/portable launchers retain adjacent-runtime/current-folder behavior.
- Preserve settings across updates. Copy portable settings only into an empty installed store and retain originals. Prepare a versioned runtime before switching the launcher; never downgrade a newer installation. Verify an update's SHA-256 before executing it.
- Check for updates at normal startup before starting Engine workers, the phone server, tray controls, shortcuts, or desktop editing. A failed check permits offline launch; once a newer release is confirmed, a missing asset, invalid digest, download failure, or installer-launch failure keeps the startup gate on Retry. Retrying must not bypass that known required update. Reach full download progress only after SHA-256 verification succeeds.
- Automatic update checks happen only at startup. The running session's Settings check remains manual, asks before installation, and must not restart with pending edits, including edits arriving from the phone during a download.
- Keep sources, embedded assets, build scripts, documentation, tests, and third-party notices in Git. Exclude generated executables, deployed Qt DLLs, `.build/`, personal settings, logs, credentials, pairing captures, and stale backups.
- Packages also exclude personal data, stale executable backups, verification executables, and fixture DLLs. `package-release.ps1` remains the optional portable ZIP packager. Preserve attribution and licenses.

## Documentation and naming

- Keep [README.md](README.md) focused on installation and user tasks. This file owns agent working rules; [docs/reference.md](docs/reference.md) owns detailed application behavior; [docs/testing.md](docs/testing.md) owns diagnostic commands.
- For behavior, architecture, or workflow changes, check this file and the relevant reference or testing guidance before finishing. Update durable rules and affected guidance in place; remove obsolete guidance. Most small implementation changes need no prose. Keep feature inventories, detailed timings, and diagnostic commands in their existing documents, without turning this file into a changelog or duplicating explanations.
- Use short nearby comments for constraints or reasons the code cannot explain. Put necessary cross-component explanations in the reference document and identify the relevant source.
- Never commit temporary plans, research notes, or agent scratch files.
- Use **Azec** for the app, windows, packages, and documentation. The checkout's historical `KingBoss2004` folder name remains only because the in-use checkout cannot be renamed. Never introduce it into application files or compatibility fallbacks.
