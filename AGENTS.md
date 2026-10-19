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
- `src/launcher.cpp` / `src/app_paths.hpp`: embedded single-file builds install under the Windows per-user Programs folder and use `%LOCALAPPDATA%\Azec` for configuration, logs, and the single-instance lock. Unembedded development/portable launchers keep the original adjacent-runtime/current-folder behavior.
- `tests/phone_test.py`: HTTP integration checks against an isolated dry-run instance.
- `tests/core_checks.cpp` / `tests/ui_checks.cpp`: separate verification code, excluded from the release executable.

## Build and verification

Use PowerShell from the project root. Requires Visual Studio 2022 C++ tools, a Windows SDK, and Qt 6.8.3 MSVC x64. `setup-qt.ps1` installs the pinned Qt kit; `NEON_QT_ROOT` can override its location.

**Run the suite only for substantial changes.** Do not automatically run `build.cmd test`, UI checks, or browser matrices after every edit. Documentation, comments, screenshot presentation, wording, and other simple low-impact edits need only relevant lightweight checks (links, syntax, diff review, or a focused visual inspection). A small executable-code change may need a compile or targeted behavior check; line count alone does not establish risk. Authentication, persistence, concurrency, and input changes warrant behavior checks even if the diff is small.

- Build-only: `.\build.cmd`.
- Substantial behavior changes: `.\build.cmd test` runs core, isolated HTTP, recovery, and restart checks. It skips the real 61-second expiry wait.
- Desktop layout/interaction changes: after building the verification executable with `build.cmd test`, run `python tests/run_checks.py --ui-only`. Default: 100% scale, no exhaustive language sweep. Inspect relevant screenshots and JSON results.
- Phone UI changes: `python tests/phone_browser_test.py`. Requires Python Playwright and Google Chrome. Default layout coverage is English at 390 CSS px; focused language-picker behavior remains checked.
- Use `--full` on either Python runner only for releases, broad layout/typography/localization changes, or a bug involving scaling/translations. UI coverage expands to 100% / 150% and all languages; browser coverage expands to nine languages at four widths. Do not run this matrix for unrelated small edits.
- Pairing expiry changes: `python tests/run_checks.py --slow` includes the real timeout. CI retains extended coverage explicitly.

See [docs/testing.md](docs/testing.md) for the complete command reference. Build intermediates belong in `.build/`; deployed binaries belong in `runtime/`. Exclude `runtime/Azec-checks.exe` from runnable releases.

All fixture checks must use fresh isolated directories under `.build/verification`; the Python runners create these automatically. Never run fixture tests from the project root or against personal configuration. Use `--dry-run` for interactive inspection and browser checks so no mouse events or beeps are generated. Pairing info and revealed QR captures contain session credentials and must remain local. Stop test processes started by your work. Report exactly what was checked and what was skipped; browser emulation is not a physical-phone test.

## Data and behavior contracts

- Preserve the original 3,734-byte primary configuration layout in `r6_config.bin`. Keep backward compatibility when adding features.
- The operator roster covers all 72 operators in id order: Ace, Alibi, Amaru, Aruni, Ash, Azami, Bandit, Blackbeard, Blitz, Brava, Buck, Capitao, Castle, Caveira, Clash, Deimos, Doc, Dokkaebi, Echo, Ela, Fenrir, Finka, Flores, Frost, Fuze, Glaz, Goyo, Gridlock, Grim, Hibana, Iana, IQ, Jackal, Jager, Kaid, Kali, Kapkan, Lesion, Lion, Maestro, Maverick, Melusi, Mira, Montagne, Mozzie, Mute, Nokk, Osa, Oryx, Pulse, Ram, Rook, Sens, Sentry, Skopos, Smoke, Solis, Striker, Sledge, Tachanka, Thatcher, Thermite, Thorn, Thunderbird, Tubarao, Twitch, Valkyrie, Vigil, Wamai, Warden, Ying, Noor. Keep this order: extend at the end only so existing records never shift.
- `r6_config.bin` serializes only the original 71 records. Primary profiles for operators past the original list (Noor, id 71) and the selected operator persist in `azec_weapons.json` (`primaryProfiles`, `selected`); the binary `selected` field still rejects indexes past the original list. Backups with 71 or 72 records import by name.
- Store secondary profiles, weapon hotkeys, the phone-access preference, the global rapid-fire switch, and the game-focus preference in `azec_weapons.json`. Favorites use `azec_ui.json`.
- Fresh configurations and the unpaired phone page default to English. Preserve saved language IDs: Portuguese is 0 and English is 1; pairing follows the desktop's saved language.
- Each operator has a primary profile and an optional independent secondary profile. First enable copies primary settings; disabling preserves secondary edits. Character selection starts on primary.
- Default weapon keys are 1 and 2 and are configurable. Global actions (open/hide, recoil toggle, panic exit) reject ordinary typing and list-navigation keys; weapon shortcuts must not interrupt text/number editing or Ctrl/Alt/Windows-key shortcuts.
- Global input and global actions are gated by the **Only in Rainbow Six** preference, on by default. When another application is active, no mouse or keyboard action is emitted. Panic is also ignored while Azec itself is active. This is an accident-prevention gate, not a claim of anti-cheat safety.
- Preserve fractional accumulation, timing, and SendInput behavior unless the requested change concerns them. Reset fractional movement when the active operator/weapon changes.
- Use Engine synchronization when accessing configuration from worker threads. Keep UI updates on the Qt GUI thread.
- Never replace, commit, or package the user's personal configuration files as defaults.

## Presentation

- Goofy is the only desktop and phone UI. Keep the single desktop layout in `src/desktop_layout.hpp`; do not add interface selectors or restart-to-change-design flows.
- Store the named color theme (`colorTheme`) and collapsed-sidebar state (`sidebarCollapsed`) in `azec_ui.json` alongside favorites. Every preference change must preserve the others. Ignore obsolete `interface` preferences and remove that key only during a successful atomic UI-preference save.
- Themes change materials only. MainWindow bindings, Engine, hotkeys, phone server, validation, persistence, and profile transfer remain shared. Preserve the 3,734-byte binary layout.

## UI contracts

- The interface follows GoofyUI (github.com/Azec-Zuckerberg/goofy-ui-library): chunky 2-3 px ink outlines, flat colour, hard offset ink shadows without blur, no gradients or glow. Retain the compact window, custom window controls, rounded corners, and raised right panel. The original palette uses a cream `#FFFAEE` canvas, white surfaces, a seamless 60 px header matching the sidebar canvas, and purple `#7248D7` accent. Yellow `#FFDD59` stays on numeric chips and stickers. Named palettes change colors only, sharing this layout. The sidebar animates between 200 px expanded and 76 px collapsed; its top **Collapse** button (icon-only when compact) is independent of the theme; a compact star shortcut appears above the operator icons when favorites exist; search stays at the bottom, and compact search expands and focuses the search field.
- Use bundled Geist for controls, operator names, and body text, and bundled Fredoka for display headings. The top-left Azec wordmark uses the bundled Press Start 2P pixel font (white pixel lettering with a hard ink outline and shadow) inside the existing header.
- Header icons share 34 px hit targets and centered 20 px vector artwork, with separate Recoil and Rapid fire status pills. Controls never react to mouse hover anywhere (header icons, rows, buttons, sliders, scrollbars, and the phone page stay flat until pressed, focused, or selected; only context-menu items still highlight on hover). Keep Save flat, without a glow, pulse, or checkmark swap; its tooltip confirms success and failed saves show a persistent banner. Keyboard focus draws a 2 px accent outline, not a background tile. Closing with unsaved edits shows the Save/Discard/Cancel prompt and plays a soft generated two-note “no” cue; a repeated close must re-surface that prompt rather than silently ignoring it, and Cancel keeps the app running. The Save/Discard/Cancel and Quick Guide dialogs use the same thick ink border and rounded corners as the hotkey dialog.
- Save stays neutral ink while saved and uses the current accent while unsaved, including after a failed save; text on the accent is chosen by luminance so it stays readable. Keep unsaved data state independent of temporary feedback. Pairing QR starts obscured, reveals on explicit click/keyboard activation, and conceals on leaving Settings, hiding the window, or changing the link. The QR textures are rebuilt from the active palette while keeping the encoded link and a dark-on-light, scannable pattern.
- Use 24 px outer window corners, 24 px panel corners, and 10-18 px button/input/card corners. Numbers are yellow mono chips, sliders have lavender pill tracks with fully round white ink-rimmed handles (the slider keeps a 20 px minimum height so the handle is never clipped top or bottom), and switches are GoofyUI face toggles (lavender off, accent on, white knob showing `-` or `:)`). Preserve the existing layout and hit targets.
- Sidebar groups and section headings render as GoofyUI stickers: slightly tilted colour chips (yellow, blue, pink, lime) with 2 px ink outlines.
- Operator rows are 52 px tall with 44 px icons and 14 px Geist names; the selected row is a lavender pill with an ink outline. Keep long names readable within the existing sidebar width.
- Favorites appear only when nonempty and matching search. Favorited operators are hidden from All operators, so each operator appears once. Removing a favorite returns it to All operators.
- Right-click favorites use a compact context-menu action, without a nested push button; hover must not highlight the whole popup. Mouse clicks on switches must not add an outer focus ring. Preserve a keyboard focus indicator.
- Operator changes fade only the identity for approximately 120 ms while controls update immediately. Settings uses a stationary 180 ms crossfade. Rapid input must settle on the last selection.
- Scrollbars reserve their width, start invisible, appear on scrolling, stay visible during dragging, and fade after three idle seconds.
- The mouse wheel over a slider, number field, or dropdown scrolls the surrounding page instead of changing a value, unless that control already has focus. Arrow keys must reach a focused operator list; the 0.1 recoil fine-tuning only applies when no list, text field, or slider owns the keyboard.
- Feedback sounds are generated inside the app. Do not use the Windows error beep or the system console bell. Closing with unsaved changes plays a soft descending two-note cue, and the cue follows the Feedback sound preference.
- Selecting an operator persists only the selection and must never mark the profile dirty. Browsing the roster is navigation, not an edit.
- Keep the two global output switches, Recoil and Rapid fire, in the header, the tray, and the phone Output card. Rapid fire is global; only the shot delay stays per operator.
- Keep appearance choices limited to the ten named Goofy palettes in `theme.hpp`: original, Pistachio, Ice blue, Lilac, Butter & ocean, Rose, Midnight violet, Forest & tangerine, Deep sea & citron, and Charcoal & blush. Apply and save palette changes immediately without editing binary profile values or changing layout. No free accent-color picker or reactive-radius slider; saved custom binary values are still normalized on load and the next save. The output model is exactly two global switches, Recoil and Rapid fire; do not reintroduce the removed Master enable gate. Settings categories are General, Appearance, Shortcuts, Phone control, and Profile backup; conceal pairing QR when leaving its category.
- Nine language entries ship in the picker with bundled flag SVG assets: English (US), Portuguese (Brasil), French, Spanish, German, Arabic, Hebrew, Simplified Chinese, Japanese; ids 0-8 are validated in `core.cpp`, `profile_transfer.hpp`, and `phone_bridge.cpp`. The Hebrew entry is intentionally a "Free Palestine" prank (Israel flag in the picker, then a Free Palestine popup with the Palestine flag and Arabic prank strings in `src/lang_he.hpp` and the phone dictionary) and must stay that way. Translators: every language table in `src/lang_*.hpp` must keep the full 123 keys in enum order; the `static_assert`s enforce the desktop count. Every `phoneTexts` map in `assets/phone.html` must cover the same UI keys, including the phone's Output card.
- Localized text must never widen the fixed 200 px sidebar: sticker chips elide (`StickerLabel`), setting hints word-wrap, and `tests/ui_checks.cpp` asserts the sidebar fits for every language in the extended checks.

## Phone interface

- Serve the self-contained phone page from the desktop app; keep it usable without external fonts, scripts, or internet access.
- The phone page uses the same ten palettes as the PC, sourced from the desktop theme tokens in the state API. A theme change on either screen saves the same UI preference and updates both, without changing profile edits. Preserve GoofyUI materials: 3 px ink outlines and hard offset shadows, tilted sticker section chips, yellow number chips, embedded Fredoka, and face toggles. It has no hover effects either.
- The first phone card is **Output** with the two live global switches, Recoil active and Rapid fire active. Do not reintroduce the Master enable gate or the per-weapon rapid toggle.
- The phone uses the PC's LAN address, not its own localhost. Show the actual address, a selectable six-digit pairing code, Copy code, Copy link, New code, and a Retry action when the server cannot start in desktop Settings.
- Rebuild the QR textures from the active palette while keeping the encoded link and a dark-on-light, scannable pattern. Copy link may include the session token; New code must rotate the token and disconnect paired phones.
- Preserve session-token authorization, pairing rate limits, host/origin checks, request-size limits, and server disable behavior.
- Validate values before applying them. Profile edits must carry explicit operator and weapon identifiers to avoid updating a different profile after concurrent selection changes.
- Save through the same persistence path as the desktop. Verify primary/secondary isolation and persistence after reopening. Pause polling while the page is hidden.
- Do not add router port forwarding, public hosting, or external telemetry as part of local phone support.

## Repository and delivery

Keep source, embedded assets, tests, build/setup scripts, documentation, and third-party license files in Git. Generated executables, deployed Qt DLLs, `.build/`, personal configurations, credentials, and pairing info are excluded by `.gitignore`.

Preserve third-party attribution and licenses. The friend-facing `share.cmd` build embeds a clean runtime and licenses in `dist\Azec.exe`, installs per user (normally `%LOCALAPPDATA%\Programs\Azec`), and creates a Start menu shortcut. Profiles/settings live separately in `%LOCALAPPDATA%\Azec`; updates must preserve them. Copy existing portable profiles only into an empty installed settings store and retain their originals. Prepare versioned runtimes before switching the launcher, never downgrade an existing newer installation, and verify the release SHA-256 before executing an update. `package-release.ps1` still creates the optional adjacent-runtime portable zip. Exclude personal settings, logs, pairing captures, stale backups, and all verification executables from both packages. Installer checks must use `AZEC_INSTALL_TEST_ROOT` with `--dry-run` to isolate Programs, data, and shortcut paths under a fresh `.build/verification` fixture. Report actual checks and limitations; never claim physical-phone or in-game tests without performing them.

### Release workflow for AI agents

When Azec asks to **build and push**, **ship a release**, **publish an update**, or **release a new version**, complete both the source delivery and the friend-facing EXE publication. That request authorizes the commit, push, and public binary release; do not ask for the same confirmation again. Honor any explicit restriction such as build-only, source-only, no push, or no publishing. A request just to edit code or documentation does not trigger a release.

1. Review the working tree and finish the requested changes. Preserve unrelated work, including already-staged changes; do not use blanket staging or include another task's edits in the release commit. Verify the source remote is `Azec-Zuckerberg/Azec` and the public binary destination is `Azec-Zuckerberg/Azec-releases`. Keep the source repository private.
2. Run the checks appropriate to the changes using the build and verification rules above. Resolve failures before publishing, and report any checks that could not run. Do not stop the user's running app or touch personal configuration to make a build pass.
3. Run `.\share.cmd publish` from the project root. This stamps `src/version.hpp`, rebuilds, packages `dist\Azec.exe`, runs the isolated installation smoke check, and uploads the EXE and `SHA256SUMS.txt` to a versioned release in `Azec-Zuckerberg/Azec-releases`. Do not substitute a source push, a build-only command, or `share.cmd quick` for this publication step.
4. Verify the exact published tag matches `src/version.hpp`. Download that release's EXE and checksum file into a fresh `.build/verification` directory and verify both the published checksum and the local `dist\Azec.exe` SHA-256 match the downloaded EXE. Confirm the stable latest-download URL serves the new release anonymously. Do not claim publication succeeded merely because the command returned success.
5. Commit and push this task's source changes to the source repository, including the version generated during publication and any relevant build scripts, embedded assets, documentation, and tests. Use the intended branch (normally `main`); never force-push or overwrite unrelated work. Publication changes the version file, so the final source commit must include that generated version. Verify the push reached the remote.
6. Report the released version, source commit, checks performed/skipped, and the stable download link: https://github.com/Azec-Zuckerberg/Azec-releases/releases/latest/download/Azec.exe. Existing users receive this release through **Settings → General → Updates → Check for updates**, with their AppData profiles preserved.

Only the clean EXE and checksum belong in the public release. Never upload private source, personal profiles, logs, credentials, pairing data, or verification executables. If authentication, permissions, checks, publication, verification, or the source push fail, report the exact incomplete step and any steps already completed; do not describe a partial delivery as fully released.

The local working folder keeps its historical `KingBoss2004` name because the in-use checkout cannot be renamed; the GitHub repository is `Azec-Zuckerberg/Azec`, and every application name, window, written file, package, and document uses `Azec`. Do not reintroduce the old name in code, files, or compatibility fallbacks.
