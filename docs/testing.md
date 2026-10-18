# Testing Azec

Choose checks for the behavior changed. The full suite is for substantial changes, not a ritual after every edit. Documentation, comments, screenshot layouts, and simple wording changes need diff/link review or visual inspection. Small code changes may need compilation or a focused check. Security, persistence, concurrency, and input changes deserve behavior checks regardless of diff size.

## Commands

Run from the repository root in PowerShell:

| Purpose | Command |
| --- | --- |
| Build and deploy, no tests | `.\build.cmd` |
| Build app + verifier; core, HTTP, recovery, restart checks | `.\build.cmd test` |
| Repeat behavior checks against an already current build | `python tests/run_checks.py` |
| Desktop checks at 100%, one interface | `python tests/run_checks.py --ui-only --interface goofy` |
| Desktop checks at 100%, both interfaces | `python tests/run_checks.py --ui-only` |
| Phone browser checks, one interface | `python tests/phone_browser_test.py --interface goofy` |
| Pairing expiry, including the real 61-second wait | `python tests/run_checks.py --slow` |
| Extended desktop matrix | `python tests/run_checks.py --ui-only --full` |
| Extended phone browser matrix | `python tests/phone_browser_test.py --full` |
| Behavior + extended desktop matrix | `python tests/run_checks.py --ui --full` |
| Decode rendered desktop QR images | `python tests/run_checks.py --ui-only --full --qr-scan` |

Use `rally` instead of `goofy` to target Rally. Omitting `--interface` checks both. `--full` expands requested checks; it does not implicitly enable desktop checks. `build.cmd test --slow` includes expiry during a build. The browser runner requires `pip install playwright` and installed Google Chrome. QR decoding requires `pip install opencv-python-headless`.

Build `runtime/Azec-checks.exe` with `build.cmd test` before running desktop checks. Rebuild after changes to C++ or embedded assets; testing a stale executable does not validate source edits. The release app delegates `--ui-self-test` to the separate verification executable.

## Fast defaults and extended coverage

The default HTTP suite still checks pairing rejection, lockout, authorized access during lockout, validation, host/origin checks, profile isolation, and persistence. Only the two checks after the real expiry wait are skipped, and the JSON report lists them explicitly. Production timeout behavior is unchanged.

Default desktop checks use 100% scaling and omit exhaustive translated-layout sweeps. Focused English/Portuguese language-picker behavior remains covered. Default browser layout coverage is English at 390 CSS px; pairing, pickers, hotkey conflicts, persistence, and expired sessions remain checked. Browser reports record the selected layout matrix.

150% scaling detects pixel rounding, clipped controls, and screen-fit errors on scaled Windows displays. All-language checks detect long labels, font differences, and right-to-left layout problems. Run the extended matrix for relevant layout/typography/translation changes or release verification, rather than for unrelated changes. It checks desktop at 100% and 150%, and phone layout in nine languages at 320, 390, 430, and 768 CSS px.

The Windows CI workflow explicitly retains the slow expiry check and extended desktop/QR matrix. The optional Chrome suite is separate. Local fast checks are not a claim that the extended matrix passed.

## Fixtures and artifacts

Python runners create fresh directories under `.build/verification`, run isolated dry-run instances, and stop their test processes. Inspect JSON reports and relevant screenshots. Never run fixture checks against the user's configuration or from the project root: the native checks write settings into their working directory.

For manual inspection, launch the absolute path to `runtime/Azec.exe` from a fresh fixture directory with `--dry-run`. For motion captures, run the verifier there with `--ui-self-test --motion-only`; add `--motion-timing-only` to omit frame capture. Fixture settings, pairing info, revealed QR images, and motion captures must remain local. CI excludes pairing-info files and revealed QR captures from uploads.

Automated browser checks are emulation, not physical iPhone or in-game testing. State actual checks and limitations in delivery notes.
