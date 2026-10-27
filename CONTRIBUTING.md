# Contributing to Azec

Azec is a native Windows application built with C++17 and Qt. Contributions can
include a reproducible bug report, a clearer setup guide, a translation fix, or
a focused code change. You do not need to understand the whole application to
start.

## Find a useful first change

Read the [README](README.md), search existing issues, and try the documented
workflow before proposing a replacement. For a small, clear bug, explain what
fails and send a focused fix. For a new feature, a redesign, or a change to saved
data or output behavior, open an issue and agree on the intended behavior before
investing in an implementation. An issue or pull request is a proposal, not a
promise that the change will be accepted.

Keep each pull request about one underlying problem. Include the desktop,
phone, persistence, and documentation changes that problem actually needs;
leave unrelated cleanup for another contribution.

## Build your first checkout

Use Windows x64 with Visual Studio 2022 C++ tools, a Windows SDK, Python, and
Qt 6.8.3 MSVC x64. In PowerShell, from the repository root:

```powershell
.\setup-qt.ps1
.\build.cmd
```

If you already have the compatible Qt kit, set `NEON_QT_ROOT` to its directory
and skip setup. Node.js is needed for the console phone checks. See
[Testing](docs/testing.md) for optional browser dependencies and focused commands.

## Learn your way around the code

Start with the component that owns the behavior you want to change:

| Area | Starting point |
| --- | --- |
| Startup, storage, and installation | `src/app.cpp`, `src/startup.hpp`, `src/launcher.cpp` |
| Desktop controls and layout | `src/main_window.hpp`, `src/desktop_layout.hpp` |
| Shared state and profile saving | `src/engine.*`, `src/profile_store.hpp` |
| Phone requests and interface | `src/phone_bridge.cpp`, `assets/phone.html` |
| Translations | `src/strings.hpp`, `src/lang_*.hpp`, phone dictionaries |

The [application reference](docs/reference.md) explains the shared behavior.
[AGENTS.md](AGENTS.md) records compatibility and development constraints for
humans and coding assistants. Follow the existing patterns and prefer the
smallest change that fixes the established problem.

## Protect profiles and the desktop

Never test against real profiles or use the live Azec data folder as a fixture.
Use fresh data under `.build/verification`. Desktop checks must run through
`tests/hidden_process.py` on its noninteractive Windows station, with `--dry-run`
to suppress mouse output and sounds. If isolation fails, stop the check. Do not
stop another running copy of Azec to make a build or test succeed.

Changes to profiles need to account for both weapons, pending Save/Discard
behavior, old backups, and the phone editing the same state. Keep the existing
binary format and operator IDs compatible. Preserve third-party notices.

## Show what changed and how you checked it

Describe the original failure, the resulting behavior, and the steps or focused
checks that demonstrate the fix. Give the Windows version and relevant build or
test results, including anything you could not verify. A successful build alone
does not demonstrate an interaction or an installation.

For interface changes, provide before-and-after screenshots; include a short
recording when motion matters. Keep pairing codes, token links, QR credentials,
personal profiles, generated binaries, and temporary captures out of the pull
request and Git history. Do not claim physical-phone, driver, or in-game testing
unless you performed it.

Coding assistants are welcome, but you remain responsible for understanding the
change and checking its behavior. Review feedback should address the code and
evidence. Maintainers may request a smaller change or decline a direction.

## Security and releases

Use [private security reporting](SECURITY.md) for vulnerabilities. Normal bug
reports belong in [Issues](https://github.com/ZbouhaKahlin/Azec/issues).

Contributors should build and test locally. Release publication is a maintainer
action on `main`; see [Releasing](AGENTS.md#releasing). Contributions are offered
under the project's [MIT license](LICENSE), with existing third-party licenses
remaining applicable.
