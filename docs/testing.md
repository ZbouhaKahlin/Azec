# Testing Azec

Tests run whenever a check answers a real question; no separate permission is needed. Keep the isolation rules below so a check never disturbs the visible desktop. The shared policy is in [AGENTS.md](../AGENTS.md).

Choose checks relevant to the problem and stop after they pass. Extra scales, languages, expiry waits, and matrices are separate coverage; run them when that coverage is the question. Tests must not open visible windows, steal focus, move the cursor, or play sounds. Desktop-backed Python runners use `hidden_process.py`: each GUI fixture runs on a noninteractive Windows station with its own desktop and a cleanup job covering descendants. The station has no access to your visible desktop or clipboard. Isolation failure stops the launch; there is no visible fallback.

## What the test files are

The files in `tests/` are development tools. They do not run when a user opens Azec. None is shipped in the friend-facing EXE or portable zip, and the verification executable is excluded from both packages.

| Files | Purpose | Uses a desktop app window? |
| --- | --- | --- |
| `core_test.cpp`, `core_checks.cpp`, `core_checks.hpp` | Standalone checks for binary compatibility, validation, movement arithmetic, and atomic saves | No |
| `run_checks.py` | Creates isolated fixtures and coordinates HTTP, recovery, restart, and optional UI checks | Yes |
| `phone_test.py` | Checks the local HTTP API, authorization, validation, and profile isolation against a running fixture | Its fixture does |
| `recovery_test.py` | Checks damaged/unreadable settings and preservation of existing data | Yes |
| `preferences_restart_test.py` | Checks profiles, favorites, obsolete-key cleanup, and old preference migration across restarts | Yes |
| `ui_checks.cpp` | Desktop layout, controls, focus, motion, dialogs, QR rendering, and optional language coverage | Yes |
| `phone_browser_test.py` | Phone-page interaction in headless Chrome against a desktop fixture | The desktop fixture does; Chrome is headless |
| `phone_companion_test.py` | Real QR-photo decoding, pairing failure/retry, compact delay layout, and muted Web Audio scheduling | The desktop fixture does; Chrome is headless and audio is muted |
| `qr_scan.py` | Decodes existing QR captures; launches no app | No |
| `installation_test.py` | Checks the actual installer, updates, migration, and installed profiles in redirected folders | Yes |
| `update_checks.cpp`, `update_checks.pro` | Separate console executable for updater downloads, hashes, and failure handling | No |
| `review_regressions.cpp`, `review_regressions.pro` | Focus-name validation, LAN ranking, persistent DLL placement, archive extraction, save rollback, and launcher repair/cleanup helpers | No |
| `reliability_checks.cpp`, `reliability_checks.pro`, `driver_fixture.cpp`, `driver_fixture.pro` | Older-backup preservation, bounded driver downloads, context retries through a test-only DLL, optional installer failures, and empty UI-preference recovery | No |
| `hidden_process.py`, `desktop_isolation_test.py` | Noninteractive GUI launch, fail-closed behavior, and complete fixture-process cleanup | Native probe only, isolated |
| `phone_api_test.cjs` | Phone request/error handling and translation coverage using Node, without a browser or app | No |

The shared core checks run through the sensitivity verifier. `core_test.cpp` is a separate standalone entry point, not built by `build.cmd`. Keeping these optional tools preserves useful diagnostics. Their presence does not make testing mandatory.

## Commands

These commands run from the repository root in PowerShell. Desktop-backed runners use a noninteractive Windows station, including native Windows rendering; this is separate from Qt offscreen emulation. `--dry-run` suppresses input and sounds. Running the application directly bypasses the isolation wrapper and can show a window, so use the documented Python/build commands for invisible checks.

| Purpose | Command |
| --- | --- |
| Build and deploy, no tests | `.\build.cmd` |
| Build app + verifier, no tests | `.\build.cmd checks` |
| Build app + all verifiers; console, HTTP, recovery, restart checks | `.\build.cmd test` |
| Complete suite: console, behavior, desktop UI and headless phone browser | `.\build.cmd test --suite` |
| Repeat the complete suite against current binaries | `python tests/run_checks.py --suite` |
| Repeat behavior checks against an already current build | `python tests/run_checks.py` |
| Switch motion, Save/Copy returns, glass clipping and move/resize corners | `python tests/run_checks.py --interaction-motion-only` |
| Phone switch and Save return motion | `python tests/phone_browser_test.py --interaction-motion-only` |
| Desktop checks at 100% | `python tests/run_checks.py --ui-only` |
| Startup loading, expansion, content reveal, and interruption captures | `python tests/run_checks.py --startup-only` |
| Native startup frame timing without screenshot overhead | `python tests/run_checks.py --startup-timing-only` |
| Startup update progress, offline launch, verified handoff, and Retry | `python tests/run_checks.py --startup-update-only` |
| Phone browser checks | `python tests/phone_browser_test.py` |
| Phone QR-photo pairing, delay layout, and sound scheduling | `python tests/phone_companion_test.py` |
| Pairing expiry, including the real 61-second wait | `python tests/run_checks.py --slow` |
| Extended desktop matrix | `python tests/run_checks.py --ui-only --full` |
| Extended phone browser matrix | `python tests/phone_browser_test.py --full` |
| Behavior + extended desktop matrix + real pairing expiry | `python tests/run_checks.py --ui --full` |
| Decode rendered desktop QR images | `python tests/run_checks.py --ui-only --full --qr-scan` |
| Package the current build as a portable ZIP | `.\package-release.ps1` |
| Build the single-file installer, no tests | `.\share.cmd` |
| Resume verification of an uploaded release | `powershell -NoProfile -File .\verify-release.ps1 -Version <version> -ResumeDirectory <printed-folder>` |
| Build and run the installer smoke test | `.\share.cmd test` |

`--full` expands requested checks; it does not implicitly enable desktop checks. In `run_checks.py`, it also enables the real pairing-expiry wait whenever HTTP behavior checks are included; `--ui-only --full` omits HTTP and that wait. `build.cmd test --slow` includes expiry during a build. The console group, including `build.cmd test`, requires Node.js with `node --test` support for `phone_api_test.cjs`. The browser runner requires `pip install playwright` and installed Google Chrome. The separate companion runner also needs `pip install qrcode pillow`; its temporary pairing QR images are deleted during cleanup. Both browser runners use `--mute-audio` so Web Audio cannot play through the current desktop. QR decoding requires `pip install opencv-python-headless` and Node.js. It checks production QR captures across representative pairing links and a longer symbol with alignment markers. Every capture is decoded unchanged and with mild blur, reduced contrast, smaller size, and perspective distortion. If OpenCV finds no payload, the bundled phone decoder must read that exact image and match its expected hash; reports retain the OpenCV misses and the decoder used. An incorrect decoded payload always fails. These synthetic checks do not establish physical-phone scanning speed.

Use `build.cmd checks` to prepare the desktop verifier and all console verifiers without running tests. Console binaries remain under `.build/checks-*/release`; only the desktop verifier goes in `runtime/Azec-checks.exe`. The test-only driver DLL is never copied into the app runtime. `build.cmd test` also builds it and explicitly runs behavior checks. Rebuild after changes to C++ or embedded assets; testing a stale executable does not validate source edits. The release app delegates `--ui-self-test` to the separate verification executable.

Ordinary dry-run fixtures skip the real startup update check and never download or execute an updater. `--startup-update-only` routes the update window's requests to a local server and uses a callback instead of running the downloaded fixture. It checks the smooth bar reset, verified download, current-version launch, offline message, corrupt and missing-hash failures, installer-launch failure, and Retry after a confirmed required update. It also checks that checking, loading, and expansion use one visible native window, keep the logo and bar continuous, and pause services and shortcuts until checking ends. It does not install or publish a real release.

## Choosing coverage

The default HTTP suite still checks pairing rejection, lockout, authorized access during lockout, validation, host/origin checks, profile isolation, and persistence. Only the two checks after the real expiry wait are skipped, and the JSON report lists them explicitly. Production timeout behavior is unchanged.

Default desktop checks use 100% scaling and omit exhaustive translated-layout sweeps. Focused English/Portuguese language-picker behavior remains covered. Default browser layout coverage is English at 390 CSS px; pairing, pickers, hotkey conflicts, persistence, and expired sessions remain checked. Browser reports record the selected layout matrix.

The optional extended matrix checks desktop at 100% and 150%, and phone layout in nine languages at 320, 390, 430, and 768 CSS px. It can diagnose scaling, clipping, or translation problems. Use it when that coverage is the question.

The Windows GitHub Actions workflow is manual only. Its default run applies behavior checks on the CI machine; desktop checks, the extended matrix, and the real expiry wait are separate input choices, all off by default. The `extended` input is ignored unless `desktop` is enabled. The optional Chrome suite is separate. A focused run is not a claim that extended coverage passed.

## What the checks cover today

Input and output behaviour is only partially automatic. The fixtures run with `--dry-run`, so no mouse movement, synthetic clicks, or sounds are produced by the engine. The native desktop windows remain inside the noninteractive fixture station. Real Interception output, the game-focus gate with a live game, and the audible sound cues remain unverified by these suites.

Available automated coverage:

- The Recoil switch updates live and starts off on launch; per-profile Rapid fire persists and reloads. Removed Master enable settings are rejected by the phone API.
- The game-focus gate recognises executable names beginning with `RainbowSix`, excluding names containing `_BE`, with a fallback for window titles containing “Rainbow Six”. This is a focus heuristic, not process authentication.
- Non-panic bindings accept any supported key; panic retains the stricter safe-key filter, and conflicting assignments are rejected.
- Browsing the roster does not mark the profile dirty, and the wheel over a number field does not change a value.
- The pairing code chip, copy actions, new-code rotation, and the server-failure retry path are exercised in the desktop and browser suites.
- The phone Output card, independent primary/secondary Rapid fire, removal of obsolete controls, and the default Dark presentation are checked in the browser suite. Light phone rendering is not covered by that suite.

Still manual or not covered: a physical iPhone, real in-game input, the audible sound cues, the acrylic blur behind the real window, and the behaviour of the game-focus gate while a real game is running. Treat these as unverified and say so in delivery notes.

## Fixtures and artifacts

Python runners create fresh directories under `.build/verification`, run isolated dry-run instances, and stop their entire process trees, including installer descendants. Independent groups use at most two workers by default (`--jobs 1` runs sequentially); native console verifiers run below normal priority, while GUI fixtures use normal priority. Console checks cover binary compatibility, sensitivity, updater integrity and the five reliability fixes. Suite JSON reports record group timings and failures. Core checks are included once through the sensitivity verifier, rather than repeated in the build root. Inspect JSON reports and relevant screenshots. Start the documented Python/build runners from the repository root; the app and native-check processes they launch must use fresh fixture working directories, because native checks write settings into their working directory. Never use the project root or personal configuration as a fixture.

For documentation captures, launch `runtime/Azec.exe --dry-run --capture-ui=<absolute-output-folder>` through `HiddenProcess` in a fresh fixture. It captures the desktop and General settings, then closes without running checks. Choose the fixture’s theme with `{"version":1,"theme":"dark"}` or `{"version":1,"theme":"light"}` in its `azec_ui.json`. These captures use example profiles and never reveal Phone control credentials. [Screenshots and the approved showcase](screenshots.md) records the matching example state, phone capture, background sources, exact composition, and logo/text positions.

Visible manual inspection is the exception: launch the absolute path to `runtime/Azec.exe` from a fresh fixture directory with `--dry-run`, and expect a window on your desktop. For motion captures, run the verifier there with `--ui-self-test --motion-only`; add `--motion-timing-only` to omit frame capture. Fixture settings, pairing info, revealed QR images, and motion captures must remain local. CI excludes pairing-info files, revealed QR captures, and full Phone control captures that can show the pairing code. Concealing the QR does not conceal the six-digit code.

`share.cmd` and `share.cmd quick` package without running tests. From clean, committed `main`, `share.cmd publish` (or `build-and-push.ps1`) also commits the generated version, pushes source, and publishes the tagged installer and checksum in this repository; tests remain opt-in. Add `test` to run `python tests/installation_test.py --smoke`; `AZEC_SKIP_TESTS=1` still overrides it. The full `python tests/installation_test.py` command covers profile persistence, migration, repair, update handoff, and operation-after-download-deletion checks. Optional `--previous path/to/Azec.exe` checks migration from an old portable release; `--installed-previous path/to/Azec.exe` checks a version change from an older per-user build and prevents downgrades. The Python runner sets `AZEC_INSTALL_TEST_ROOT` and passes `--dry-run` to every installed fixture executable; it takes no `--dry-run` Python option. The launcher honors the redirect only with that executable flag, so the variable alone is insufficient. Fixture PATH excludes the development Qt kit. Never run an installation fixture without both protections. `package-release.ps1` remains the optional portable zip packager and excludes verification executables, stale builds, and personal data.

Automated browser checks are emulation, not physical iPhone or in-game testing. State actual checks and limitations in delivery notes.

## Console checks for stored sensitivity compatibility

`tests/sensitivity_checks.pro` builds `Azec-sensitivity-checks.exe` with Qt Core only, without opening the desktop app. Build it using the pinned Qt qmake and nmake in `.build/sensitivity-checks`. Run `release/Azec-sensitivity-checks.exe <fresh-empty-fixture>` with the Qt kit's `bin` on PATH and an absolute, newly created fixture directory under `.build/verification`.

This suite runs the existing core checks plus targeted coverage for INI parsing, unchanged game files, DPI reference values, per-axis rescaling, preserved secondary profiles, rejection without partial changes, sidecar and Engine persistence, backward-compatible backups, omitted machine paths, and preservation of UI preferences. It never starts input workers, opens a game, moves the cursor, or plays sounds. Do not include this verification executable in a release package.

## Update integrity checks

Build `tests/update_checks.pro` with the pinned Qt qmake and nmake in `.build/update-checks`. Run the resulting `release/Azec-update-checks.exe` from `.build/verification`; it creates its own fresh temporary directory. It checks valid, corrupt, truncated, short, HTTP-error, missing-hash, and unwritable-file downloads against a local HTTP server. Add `--live-github` to query the real public release and download/verify its EXE without executing it. For deployment verification, copy this check executable into a fresh fixture containing the actual packaged runtime and run with only the system PATH. This is a separate verification executable and must never be included in the shared payload.

## Focused review regressions

Build `tests/review_regressions.pro` with qmake and nmake in a separate `.build/` folder. Run `Azec-review-checks.exe <fresh-empty-fixture> --dry-run`, using an absolute fixture path under `.build/verification`. It uses Qt Core/Network only, never invokes the launcher entry point or input workers, and runs archive tools without console windows. It exercises normal and PowerShell extraction with apostrophes, spaces, and dollar signs, same-size DLL replacement, rollback when each configuration file is locked, required runtime DLL detection, driver migration, and preservation of newer runtimes. It does not launch or smoke-test an installed app, render the desktop, or establish in-game behavior.

Run `node --test tests/phone_api_test.cjs` for the phone request wrapper and translated error keys. Its mocked network requests exercise the actual embedded request function; they do not replace HTTP integration or physical-phone checks.

## Focused reliability checks

Build `tests/driver_fixture.pro` and then `tests/reliability_checks.pro` with qmake and nmake in a separate `.build/` folder. Both outputs go into that folder's `release/` directory. Run `release/Azec-reliability-checks.exe <fresh-empty-fixture> --dry-run` with an absolute new fixture under `.build/verification` and the pinned Qt kit's `bin` on PATH. The console runner loads only the explicitly named `Azec-driver-fixture.dll`, never the real Interception DLL, and sends no input. It also checks distinct context/movement/button failures, captured Windows errors, connected-slot selection for the reported 12/13/16 layout, slot 11 and slot 20 compatibility, no-mouse rejection/retry, populated-slot reporting, repeated-error suppression, successful sends, and report redaction. Its download cases use a local fixture HTTP server and short timeouts; launcher checks call helpers without starting the installer or desktop app. Neither the check executable nor the test-only DLL belongs in a release.

For the focused error-notice check, build the verification executable and use `hidden_process.py` to run it with `--ui-self-test --error-report-only --dry-run` from a fresh fixture. Its `ui-verification.json` covers report copying on the isolated clipboard, distinct codes, language-independent codes, replacement of old report details, persistent errors, logging, and unchanged profiles. It never opens the real driver or sends input.
