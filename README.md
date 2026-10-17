# KingBoss2004

Private Windows x64 desktop application with operator profiles, optional primary/secondary weapon settings, favorites, and paired phone controls.

## Quick start

- **Run:** download and extract the Windows package from [Releases](https://github.com/Azec-Zuckerberg/KingBoss2004/releases/latest), then open `KingBoss2004.exe`. Keep its `runtime` directory alongside it.
- **Build from source:** clone this repository, install Visual Studio 2022 C++ tools and the Windows SDK, run `setup-qt.ps1`, then run `.\build.cmd test`.
- **Phone:** open desktop Settings → Phone control and use the displayed address and pairing code in Safari on the same Wi-Fi.
- **Contributing agents:** read [AGENTS.md](AGENTS.md). [CLAUDE.md](CLAUDE.md) imports the same instructions.

The repository contains source, embedded assets, tests, scripts, and licenses. Generated application files are distributed through Releases. Personal profiles and temporary pairing credentials are excluded from Git and release packages.

## Overview

C++ / Qt 6 desktop interface for the recovered R6 behavior. The window is 864 × 650 at 100% display scale, with screen-fit sizing at higher scales, rounded outer corners, a custom title bar, a 200 px operator sidebar, and Settings inside the main window.

The title bar, sidebar, and main background share charcoal `#141416`, with control rows `#222225` and coral-red `#FF5B63` accents. Both pages live inside one stationary rounded panel with a slightly lighter charcoal surface (`#1C1C1F`), a soft painted shadow, and a fading edge highlight. Settings navigation crossfades over 180 ms. Controls use bundled 14 px Inter; operator rows are 52 px tall with 16 px Inter names and 44 px icons. The King Boss wordmark is actual seven-line ASCII artwork in red, rendered with Consolas inside the existing header. All header controls use centered 20 px vector icons inside matching 34 px hit targets, with a subtle 140 ms ink-only hover fade. Clicking switches does not add an outer focus ring; keyboard navigation retains a small focus indicator. Switches animate over 160 ms; sliders use pill-shaped tracks and round white handles. Window corners have a 22 px radius, panel corners 24 px, and buttons and inputs use softer 10-14 px corners. Reactive radius remains available in Appearance. Operator changes apply a gentle 120 ms fade only to the new icon and name; the selected profile and controls update immediately, including during rapid switching.

## Run

Double-click **KingBoss2004.exe**, or **Run.cmd**.

Keep the **runtime** directory alongside the launcher. It contains the actual application and the Qt libraries it needs; Qt does not need to be installed to run this packaged copy on this computer. The standard Visual C++ runtime is required on another computer.

- **Insert:** show or hide the window.
- **F6:** toggle recoil activity; starts OFF.
- **F8:** toggle global rapid fire; individual operator rapid fire starts OFF.
- **End:** exit.
- **1 / 2:** select primary / secondary weapon when separate profiles are enabled for that operator. Both keys can be changed in Settings.
- Arrow keys make 0.1-step recoil adjustments while this window is active and a text/number field is not being edited.

Click an operator's icon/name to edit its profile. Right-click an operator for a compact Favorite / Remove favorite context-menu action with an outlined star. There is no nested push button; the action's quiet hover stays inset from the menu edge. Favorited operators move out of All operators into Favorites and return when unfavorited; each operator appears once. Both groups share one scrolling sidebar and follow the search field. The entire Favorites group is hidden when empty or when none match the search, with no placeholder or reserved space. Favorites save automatically to `kingboss_ui.json` beside the launcher and reload on startup.

Enable **Separate secondary profile** on an operator to add independent secondary recoil, humanization, rapid-fire, and delay settings. The first secondary profile starts as a copy of that operator's primary settings. Use the Primary / Secondary buttons or the configurable 1 / 2 keys to switch. Disabling secondary mode restores the primary profile and keeps the secondary edits for later. Selecting another operator starts on primary. Weapon shortcuts ignore number/text editing and Ctrl/Alt/Windows-key combinations.

The Save button stores primary settings in the original compatible `r6_config.bin` and secondary profiles, weapon keys, and the phone-access preference in `kingboss_weapons.json`. Existing primary settings are preserved.

## Phone control

The app starts a local HTTP server automatically. In desktop **Settings → Phone control**, find its Wi-Fi address and six-digit pairing code. On an iPhone connected to the same Wi-Fi, open that address in Safari and enter the code. **Copy link** includes a session token to skip code entry. The address normally uses port 18764; the app selects another available port if needed, and Settings shows the actual address. `localhost` on the phone refers to the phone itself, so use the PC address shown in Settings.

The phone page edits the same in-memory profiles and global settings as the desktop, including favorites and both weapon slots. Changes apply immediately; use **Save changes** to persist them. It refreshes desktop changes while you are not editing an input. It has no external fonts, libraries, or internet dependency. Keep the PC app running. If Windows requests network permission, allow the app on the private network used by the phone.

Phone access can be disabled in Settings. Pairing codes and tokens change when the app restarts. API calls require the session token, pairing attempts are rate-limited, and foreign origins/hosts are rejected. The app does not configure router forwarding or expose a public service. This HTTP control page is intended for a trusted local network.

The sidebar, operator controls, and Settings use fading scrollbars. They start invisible, fade in when scrolling, and fade out after three seconds without scrolling. Dragging keeps them visible until three seconds after release. Their narrow lanes remain reserved so fades never move the content.

The Save icon beside the gear persists configuration and stays flat with no glow or pulse, keeping the same icon and transparent background. Its tooltip confirms a successful save. Unsaved changes tint its ink; save failures use a distinct warning tint and tooltip. The main window has no footer strip. The header gear opens an inline Settings page for hotkeys, language, sound, appearance, and the quick guide. The back arrow, gear, or an operator selection returns to the operator page. The open / hide window binding changes the opening shortcut. The header shows only Recoil off / on; its toggle hotkey remains configurable in Settings. Changing operators preserves unsaved profile edits in memory. Hotkey assignments save immediately, matching the recovered behavior.

Settings use `r6_config.bin` in the launcher directory. Existing valid files remain compatible. The former default neon green and teal are migrated in memory to coral red; other custom colors are retained. Reset appearance restores coral red and the original reactive radius. Optional `titio.png` can be placed in the same directory.

The supplied operator folder provides 70 matching icons. Ace uses its real icon from [r6operators](https://github.com/marcopixel/r6operators/tree/master/operators/ace). All 71 SVG icons are embedded, without generated tiles or initials placeholders; the Downloads folder is not needed at runtime. All 71 operators support an optional secondary weapon profile.

Desktop text uses embedded Inter fonts, with Windows Consolas for ASCII artwork and connection details. Fonts work without installation or internet access. Font and additional icon licenses are retained in `licenses/Inter` and `licenses/r6operators`.

## Mouse output

Movement uses **Windows SendInput**, with `MOUSEEVENTF_MOVE` and relative integer X/Y deltas. Rapid fire uses the same API with left-button up/down events. The original binary's recovered workers also use SendInput.

The recovered fractional accumulation, shared X/Y humanization jitter, 12/30 ms recoil intervals, and rapid-fire sequence (up → 5 ms → down → 25 ms → up → configured delay with a 30 ms floor) remain in the C++ engine. The original Logitech DLL lookup is retained; its resolved export is not called.

## Build

Requires Visual Studio 2022 C++ tools, a Windows SDK, and Qt 6.8.3 MSVC x64. The Qt SDK is already installed for this machine under `%LOCALAPPDATA%\NeonManagerR6\Qt`.

On another development machine, run `setup-qt.ps1` with Python available. It downloads a pinned Qt kit using aqtinstall into the user's local application-data directory. Alternatively, set `NEON_QT_ROOT` to a compatible Qt kit.

```powershell
.\build.cmd
.\build.cmd test
```

The second command also runs the core checks. Build intermediates go in `.build`. The build deploys the Qt runtime and compiles the small root launcher. Full application source is in `src`; operator SVGs and the embedded-resource manifest are in `assets`.

- `src/app.cpp`: window, settings, hotkeys, dialogs, and UI verification.
- `src/ui_widgets.hpp`: switches, SVG rendering, sidebar rows, and section cards.
- `src/engine.cpp` / `engine.hpp`: input workers, active weapon selection, and synchronized configuration access.
- `src/profile_store.hpp`: compatible secondary-profile persistence.
- `src/phone_server.hpp` and `assets/phone.html`: paired local HTTP control server and mobile interface.
- `src/core.cpp` / `core.hpp`: original binary record layout and movement arithmetic.
- `src/launcher.cpp`: starts the packaged app with the correct working directory.

Qt is dynamically linked. Its license texts are in `licenses/Qt`; upstream source for the pinned release is available from [Qt Base](https://code.qt.io/cgit/qt/qtbase.git/tree/?h=v6.8.3) and [Qt SVG](https://code.qt.io/cgit/qt/qtsvg.git/tree/?h=v6.8.3). Deployment uses [windeployqt](https://doc.qt.io/qt-6/windows-deployment.html).

## Verification

The redesigned desktop passed nine core checks, 85 UI checks at both 100% and 150% display scaling, and 24 HTTP/restart checks. Core checks cover the compatible primary layout, movement arithmetic, timing, and independent secondary profiles. UI checks cover aligned header icons, hover pixels without background tiles, static save feedback without a glow or icon replacement, keyboard-only button focus, readable operator names, bundled typography, default-color migration with custom colors preserved, favorites without duplicates, inset menu hover, empty-group behavior, Settings transitions, rapid selection, all 71 icons, profile isolation, 1 / 2 switching, typing and modifier guards, secondary enable/disable behavior, save/reload, and scrollbar timing/dragging without layout changes.

Phone checks cover pairing, authorization, invalid values/slots, foreign-origin rejection, profile edits, primary/secondary isolation, icons, saving, restart persistence, and reachability through the PC's LAN address. Browser checks at phone width exercise the real pairing form, weapon buttons, numeric edits, favorites, saving, unique operator options, and reconnecting with a new session link. The native launcher and runtime were inspected using Windows PrintWindow capture. A physical iPhone and real mouse output/in-game behavior were not tested.

UI verification disables all SendInput calls:

```powershell
New-Item -ItemType Directory -Force .build\verification | Out-Null
Push-Location .build\verification
& '..\..\runtime\KingBoss2004.exe' --ui-self-test
Pop-Location
```

The test writes screenshots, a JSON report, and fixture settings in its working directory. Use that isolated directory so fixture writes cannot replace personal settings. `--dry-run --exit-after-ms=6000` opens the interface without mouse output or beeps.

This remains a reconstruction, not the original developer's exact source tree. Real mouse output, Logitech hardware, and in-game behavior have not been tested; source-level or global behavioral equivalence is not claimed.
