# Testing Azec

Choose checks for the behavior changed. The full suite is for substantial changes, not a ritual after every edit. Documentation, comments, screenshot layouts, and simple wording changes need diff/link review or visual inspection. Small code changes may need compilation or a focused check. Security, persistence, concurrency, and input changes deserve behavior checks regardless of diff size.

## Commands

Run from the repository root in PowerShell:

| Purpose | Command |
| --- | --- |
| Build and deploy, no tests | `.\build.cmd` |
| Build app + verifier; core, HTTP, recovery, restart checks | `.\build.cmd test` |
| Repeat behavior checks against an already current build | `python tests/run_checks.py` |
| Desktop checks at 100% | `python tests/run_checks.py --ui-only` |
| Phone browser checks | `python tests/phone_browser_test.py` |
| Pairing expiry, including the real 61-second wait | `python tests/run_checks.py --slow` |
| Extended desktop matrix | `python tests/run_checks.py --ui-only --full` |
| Extended phone browser matrix | `python tests/phone_browser_test.py --full` |
| Behavior + extended desktop matrix | `python tests/run_checks.py --ui --full` |
| Decode rendered desktop QR images | `python tests/run_checks.py --ui-only --full --qr-scan` |
| Build a runnable package | `.\package-release.ps1` |

`--full` expands requested checks; it does not implicitly enable desktop checks. `build.cmd test --slow` includes expiry during a build. The browser runner requires `pip install playwright` and installed Google Chrome. QR decoding requires `pip install opencv-python-headless`.

Build `runtime/Azec-checks.exe` with `build.cmd test` before running desktop checks. Rebuild after changes to C++ or embedded assets; testing a stale executable does not validate source edits. The release app delegates `--ui-self-test` to the separate verification executable.

## Fast defaults and extended coverage

The default HTTP suite still checks pairing rejection, lockout, authorized access during lockout, validation, host/origin checks, profile isolation, and persistence. Only the two checks after the real expiry wait are skipped, and the JSON report lists them explicitly. Production timeout behavior is unchanged.

Default desktop checks use 100% scaling and omit exhaustive translated-layout sweeps. Focused English/Portuguese language-picker behavior remains covered. Default browser layout coverage is English at 390 CSS px; pairing, pickers, hotkey conflicts, persistence, and expired sessions remain checked. Browser reports record the selected layout matrix.

150% scaling detects pixel rounding, clipped controls, and screen-fit errors on scaled Windows displays. All-language checks detect long labels, font differences, and right-to-left layout problems. Run the extended matrix for relevant layout/typography/translation changes or release verification, rather than for unrelated changes. It checks desktop at 100% and 150%, and phone layout in nine languages at 320, 390, 430, and 768 CSS px.

The Windows CI workflow explicitly retains the slow expiry check and extended desktop/QR matrix. The optional Chrome suite is separate. Local fast checks are not a claim that the extended matrix passed.

## What the checks cover today

Input and output behaviour is only partially automatic. The fixtures run with `--dry-run`, so no mouse movement, synthetic clicks, or sounds are ever produced. That keeps the suites safe, but it also means the real SendInput path, the game-focus gate, and the generated sound cues are verified by reading the code and by unit-level checks, not by the suite.

Covered automatically today:

- The Recoil switch updates live and starts off on launch; per-profile Rapid fire persists and reloads. Removed Master enable settings are rejected by the phone API.
- The game-focus gate recognises `RainbowSix*.exe` and rejects unrelated processes, including the BattlEye helper.
- Show/hide, recoil-toggle and panic bindings reject typing and list-navigation keys.
- Browsing the roster does not mark the profile dirty, and the wheel over a number field does not change a value.
- The pairing code chip, copy actions, new-code rotation, and the server-failure retry path are exercised in the desktop and browser suites.
- The phone Output card, the removal of the old master and per-weapon rapid controls, and the shared palette are checked in the browser suite.

Still manual or not covered: a physical iPhone, real in-game input, the audible sound cues, and the behaviour of the game-focus gate while a real game is running. Treat these as unverified and say so in delivery notes.

## Fixtures and artifacts

Python runners create fresh directories under `.build/verification`, run isolated dry-run instances, and stop their test processes. Inspect JSON reports and relevant screenshots. Never run fixture checks against the user's configuration or from the project root: the native checks write settings into their working directory.

For manual inspection, launch the absolute path to `runtime/Azec.exe` from a fresh fixture directory with `--dry-run`. For motion captures, run the verifier there with `--ui-self-test --motion-only`; add `--motion-timing-only` to omit frame capture. Fixture settings, pairing info, revealed QR images, and motion captures must remain local. CI excludes pairing-info files and revealed QR captures from uploads.

`share.cmd` builds the friend-facing single-file installer and runs `python tests/installation_test.py --smoke`. Run `python tests/installation_test.py` for profile persistence, migration, repair, update handoff, and operation after deleting the download. Optional `--previous path/to/Azec.exe` checks migration from an old portable release; `--installed-previous path/to/Azec.exe` checks a version change from an older per-user build and prevents downgrades. All installer fixtures redirect Programs, data, and the Start menu with `AZEC_INSTALL_TEST_ROOT` plus `--dry-run`; their PATH excludes the development Qt kit. Never run an installation fixture without this redirection. `package-release.ps1` remains the optional portable zip packager and excludes verification executables, stale builds, and personal data.

Automated browser checks are emulation, not physical iPhone or in-game testing. State actual checks and limitations in delivery notes.

## Console checks for mouse setup

`tests/sensitivity_checks.pro` builds `Azec-sensitivity-checks.exe` with Qt Core only, without opening the desktop app. Build it using the pinned Qt qmake and nmake in `.build/sensitivity-checks`. Run `release/Azec-sensitivity-checks.exe <fresh-empty-fixture>` with the Qt kit's `bin` on PATH and an absolute, newly created fixture directory under `.build/verification`.

This suite runs the existing core checks plus targeted coverage for INI parsing, unchanged game files, DPI reference values, per-axis rescaling, preserved secondary profiles, rejection without partial changes, sidecar and Engine persistence, backward-compatible backups, omitted machine paths, and tour-completion preferences. It never starts input workers, opens a game, moves the cursor, or plays sounds. It does not verify the tour's rendered layout, animation, or native focus behavior. Do not include this verification executable in a release package.

## Update integrity checks

Build `tests/update_checks.pro` with the pinned Qt qmake and nmake in `.build/update-checks`. Run the resulting `release/Azec-update-checks.exe` from `.build/verification`; it creates its own fresh temporary directory. It checks valid, corrupt, truncated, short, HTTP-error, missing-hash, and unwritable-file downloads against a local HTTP server. Add `--live-github` to query the real public release and download/verify its EXE without executing it. For deployment verification, copy this check executable into a fresh fixture containing the actual packaged runtime and run with only the system PATH. This is a separate verification executable and must never be included in the shared payload.
