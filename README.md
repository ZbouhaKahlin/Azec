# Azec

A Windows desktop app with 72 operator profiles, independent primary and secondary weapon settings, and phone controls over your local network.

![Azec desktop and phone controls side by side](docs/showcase.png)

*Real application screenshots in illustrative desktop and iPhone-style frames. The phone frame is not a specific hardware model.*

## Get started

1. Download and extract the Windows x64 package from [Releases](https://github.com/Azec-Zuckerberg/Azec/releases/latest).
2. Open **Azec.exe**. Keep the included `runtime/` folder beside it; Qt is bundled. Windows needs the Visual C++ runtime.
3. Select an operator, adjust its settings, and click **Save**. Recoil activity starts off.

### Connect your phone

Keep the PC and phone on the same local network. Open **Settings → Phone control** on the PC, then open the displayed address in your phone browser and enter the pairing code. You can also explicitly reveal and scan the QR code. Keep Azec running on the PC.

Phone edits update the same profiles as the desktop. Tap **Save changes** to keep them after restarting. No external fonts, scripts, or cloud service are required. If Windows prompts, allow network access on your private network. Use the PC address shown in Settings, not `localhost` on your phone.

## Make it yours

- **Goofy color themes:** choose the original palette or Pistachio, Ice blue, Lilac, Butter & ocean, Rose, Midnight violet, Forest & tangerine, Deep sea & citron, and Charcoal & blush in **Settings → Appearance**. Colors apply immediately and save automatically; every theme keeps the same layout, with a spacious header matching the sidebar. [See all ten themes](docs/theme-gallery.md).
- **Collapsible sidebar:** the **Collapse** button at the top of the Goofy sidebar switches between operator names and a compact icon rail. Search stays at the bottom; click its compact icon to expand smoothly and start searching. The sidebar remembers your choice.
- **Settings categories:** General, Appearance, Shortcuts, Phone control, and Profile backup keep related options together.
- **Independent profiles:** enabling a secondary profile copies the primary once, then keeps edits separate. Disabling it preserves those edits.
- **Favorites and search:** right-click an operator to favorite it. Favorites appear once and save automatically.
- **Portable backups:** export or import profiles in **Settings → Profile backup**. Imported profile changes remain unsaved until you click Save.
- **Phone pairing:** sessions rotate on restart. Phone access can be disabled in Settings.

## Default shortcuts

| Key | Action |
| --- | --- |
| Insert | Show / hide the window |
| F6 | Toggle recoil activity |
| 1 / 2 | Select primary / secondary when enabled |
| Arrow keys | Adjust recoil by 0.1 while the window is active, outside text/number editing |
| End | Immediate exit; unsaved edits are discarded (ignored while you are editing a text or number field) |

Weapon and action bindings are configurable in Settings. Ordinary closing offers **Save / Discard / Cancel** when there are unsaved edits.

## Build from source

Requires Windows x64, Visual Studio 2022 C++ tools, a Windows SDK, Python, and Qt **6.8.3 MSVC x64**.

```powershell
.\setup-qt.ps1
.\build.cmd
```

`setup-qt.ps1` installs the pinned Qt kit. Set `NEON_QT_ROOT` to use another compatible installation. Build intermediates live in `.build/`; the launcher is `Azec.exe`, with deployed files in `runtime/`.

Testing is proportional to the change. Documentation and other trivial edits do **not** require the full suite. For substantial behavior changes:

```powershell
.\build.cmd test
```

This runs core, HTTP, recovery, and restart checks without the 61-second expiry wait. Desktop and browser checks are opt-in; 150% scaling and the complete language matrix are extended checks. See [Testing](docs/testing.md) for commands and when to use them.

## Documentation

- [Testing and verification](docs/testing.md) — fast defaults, targeted checks, extended coverage, and safe fixtures.
- [Application reference](docs/reference.md) — detailed UI, phone pairing, persistence, and input behavior.
- [Agent instructions](AGENTS.md) — architecture, compatibility contracts, and change-based testing rules. [CLAUDE.md](CLAUDE.md) shares these instructions.
- [Screenshot presentation](docs/showcase.html) — editable device-frame layout used for the README image.

Personal settings (`r6_config.bin`, `azec_weapons.json`, `azec_ui.json`), logs, pairing credentials, and generated binaries stay out of Git. Runnable releases contain the launcher, `runtime/`, and licenses; they exclude `Azec-checks.exe` and personal data.

## Attribution and limitations

Fonts, operator icons, country flags, Qt, and bundled QR code generation retain their notices in [licenses](licenses/). Qt is dynamically linked; pinned upstream sources are [Qt Base](https://code.qt.io/cgit/qt/qtbase.git/tree/?h=v6.8.3) and [Qt SVG](https://code.qt.io/cgit/qt/qtsvg.git/tree/?h=v6.8.3). Goofy follows [GoofyUI](https://github.com/Azec-Zuckerberg/goofy-ui-library).

Azec is a reconstruction. Browser emulation and automated checks do not establish physical iPhone compatibility or in-game behavior. Real mouse output, hardware behavior, and in-game latency have not been verified.
