# Application reference

Detailed interface, persistence, phone, and input behavior. For setup, see the [README](../README.md).

## Overview

C++ / Qt 6 desktop interface for the recovered R6 behavior. The Goofy window is 864 × 664 at 100% display scale, with screen-fit sizing at higher scales, custom window controls, and Settings inside the main window. Goofy has a 200 px operator sidebar.

The default interface follows GoofyUI (github.com/Azec-Zuckerberg/goofy-ui-library): a cream `#FFFAEE` canvas with a seamless 60 px header matching the sidebar, white surfaces with chunky ink (`#31204C`) outlines, flat colour, and hard offset ink shadows instead of blur. Live state uses the fixed GoofyUI purple accent (`#7248D7`). Both pages live inside one stationary rounded white panel with a 3 px ink outline. Settings navigation crossfades over 180 ms. Controls use bundled Geist (13 px base, 14 px rows) with Fredoka display headings; operator rows are 52 px tall with 14 px Geist names and 44 px icons, and selection is a lavender pill with an ink outline. Group and section headings are GoofyUI stickers: slightly tilted colour chips with ink outlines. The Azec wordmark is rendered in the bundled Press Start 2P pixel font — chunky white pixel lettering with a hard ink shadow inside the spacious header. All header controls use centered 20 px vector icons inside matching 34 px hit targets and stay flat until pressed, focused, or selected. Clicking switches does not add an outer focus ring; keyboard navigation keeps a 2 px accent outline. Switches are GoofyUI face toggles — lavender when off, accent when on, with a white knob showing `-` or `:)`. Sliders use lavender pill tracks with white ink-rimmed handles, and numeric fields are yellow mono chips. Window corners have a 24 px radius, panel corners 24 px, and buttons and inputs use 10-18 px corners. Operator changes apply a gentle 120 ms fade only to the new icon and name; the selected profile and controls update immediately, including during rapid switching.

## Appearance and settings

Goofy is the only desktop and phone presentation. Older interface preferences are ignored and removed on the next successful UI-preference save, preserving themes, favorites, and the collapsed-sidebar preference.

Settings groups its controls into General, Appearance, Shortcuts, Phone control, and Profile backup. Changing categories retains edits and conceals any revealed pairing QR when leaving Phone control.

Goofy offers ten fixed color palettes in Appearance: its original colors, Pistachio & coral, Ice blue & cobalt, Lilac, Butter & ocean, Rose, Midnight violet, Forest & tangerine, Deep sea & citron, and Charcoal & blush. These change the canvas, surfaces, header, accent, selection, and text colors while retaining one layout. Theme changes apply immediately and save automatically; they do not change weapon settings or require a restart. The header always matches the sidebar canvas, without a divider.

The **Collapse** button at the top of Goofy's sidebar animates between its expanded 200 px width and a compact 76 px icon rail. Operator names remain available as tooltips. Search stays at the bottom. Its icon in the compact rail expands the sidebar and focuses the search field after the animation. Favorites, filtering, selection, and right-click actions work in either state. Rapid toggles reverse from the current width. `colorTheme` and `sidebarCollapsed` persist in `azec_ui.json`, preserving favorites; invalid or missing values fall back to the original theme and expanded sidebar. Failed writes retain the previous state and show an error.

The desktop layout is built in `src/desktop_layout.hpp`. Bindings and application services remain in `MainWindow`.

![Appearance settings](ui-appearance.png)

## Screenshots

![Azec desktop interface](ui-desktop.png)

The desktop window: a seamless header with the pixel-font Azec wordmark, the 200 px operator sidebar, and operator settings with GoofyUI sticker headings and fully round slider handles.

![Azec phone page](ui-phone.png)

The paired phone page reuses the same GoofyUI materials: cream canvas, white cards with chunky ink outlines and hard offset shadows, sticker section chips, yellow number chips, and face toggles.

![Hotkey dialog](ui-hotkey-dialog.png)

The hotkey dialog keeps the action and key instructions readable, with explicit cancellation.

![Phone language picker](ui-phone-language-picker.png)

The phone language picker stays inside the page and follows the same GoofyUI materials.

## Run

Double-click **Azec.exe**, or **Run.cmd**.

Keep the **runtime** directory alongside the launcher. It contains the actual application and the Qt libraries it needs; Qt does not need to be installed to run this packaged copy on this computer. The standard Visual C++ runtime is required on another computer.

- **Insert:** show or hide the window.
- **F6:** toggle recoil activity; starts OFF. The **Master enable** switch on the Recoil control card gates every operator.
- **End:** immediate panic exit, including while a dialog is open; unsaved edits are discarded. Ignored while an Azec text or number field is being edited.
- **1 / 2:** select primary / secondary weapon when separate profiles are enabled for that operator. Both keys can be changed in Settings.
- Arrow keys make 0.1-step recoil adjustments while this window is active and a text/number field is not being edited.

Click an operator's icon/name to edit its profile. Right-click an operator for a compact Favorite / Remove favorite context-menu action with an outlined star. There is no nested push button; the action's quiet hover stays inset from the menu edge. Favorited operators move out of All operators into Favorites and return when unfavorited; each operator appears once. Both groups share one scrolling sidebar and follow the search field. The entire Favorites group is hidden when empty or when none match the search, with no placeholder or reserved space. Favorites save automatically to `azec_ui.json` beside the launcher and reload on startup.

Enable **Separate secondary profile** on an operator to add independent secondary recoil, humanization, rapid-fire, and delay settings. The first secondary profile starts as a copy of that operator's primary settings. Use the Primary / Secondary buttons or the configurable 1 / 2 keys to switch. Disabling secondary mode restores the primary profile and keeps the secondary edits for later. Selecting another operator starts on primary. Weapon shortcuts ignore number/text editing and Ctrl/Alt/Windows-key combinations.

The weapon tabs expand/collapse with a 220 ms height and opacity transition, and their selection highlights fade over 160 ms. Profile changes apply immediately. Repeated toggles reverse from the displayed position and settle on the last choice.

The Save button stores primary settings in the original compatible `r6_config.bin` and secondary profiles, weapon keys, the phone-access preference, the master recoil switch, and newer-operator profiles and selection in `azec_weapons.json`. Primary saves write and flush a temporary file before replacing the destination; the JSON sidecar is prepared first and only committed after the binary write succeeds. Rejected primary configurations are preserved as `r6_config.corrupt-<timestamp>.bin` with a visible notice. Read/preservation failures block saving to protect the existing file.

Only one instance may use a configuration directory. Launching another copy brings the existing window forward. Separate fixture directories remain independent. A tray icon offers Open/hide, Recoil active, and Exit. Ordinary closing asks Save / Discard / Cancel when changes are unsaved; failed saves keep the app open. Every nonempty hotkey must be unique across the five bindable actions (open/hide, recoil toggle, panic exit, primary weapon, secondary weapon).

Settings → Profile backup exports all primary profiles, secondary profiles, favorites, and persisted preferences into one versioned JSON file. Import validates the complete backup before applying it. Imported profile settings stay unsaved until Save is clicked; favorites retain their normal automatic persistence.

## Phone control

The advertised pairing address prefers an active physical Wi-Fi or Ethernet adapter with a gateway. Internal virtual switches without a gateway are excluded, so Hyper-V/WSL adapters cannot displace the PC's reachable LAN address. Both Copy link and the QR use this selection.

Desktop Settings uses a GoofyUI language popup and a framed hotkey dialog with wrapped instructions and Cancel. The phone uses matching language, operator, and hotkey pickers, with search for keys/operators, keyboard navigation, and Escape to dismiss. Duplicate hotkeys keep the previous binding and show the validation error without marking a working connection offline. Fresh settings and the unpaired phone page default to English; the paired page follows the saved desktop language. Existing preferences and language IDs remain compatible (Portuguese 0, English 1).

The app starts a local HTTP server automatically. In desktop **Settings → Phone control**, find its Wi-Fi address and six-digit pairing code. On an iPhone connected to the same Wi-Fi, open that address in Safari and enter the code. **Copy link** includes a session token to skip code entry. The address normally uses port 18764; the app selects another available port if needed, and Settings shows the actual address. `localhost` on the phone refers to the phone itself, so use the PC address shown in Settings.

The QR card starts blurred. Click it, or focus it and press Space, to reveal the same session link used by Copy link. Click again to conceal it. Leaving Settings, hiding the window, or changing the network address conceals it automatically. QR generation is entirely local, using bundled Project Nayuki code. The host allow-list refreshes for each request; Settings and QR address changes are detected every 2.5 seconds.

The QR uses rounded ink modules, purple finder centers, and the Azec Steady Spark logo in its center. The highest QR error-correction level protects the small logo window, and the four-module white scan margin stays clear. Module edges align to physical display pixels. Click reveal/conceal animates the colour-preserving blur over 420 ms and can reverse mid-animation. Blur textures are prepared off the GUI thread, with a gradual detail reveal and no image filtering during painting. Automatic concealment is immediate. Phone-access toggles expand/collapse only the connection details over 300 ms without rebuilding Settings or resetting its scroll position; disabling access stops the server immediately. The transition uses a transparent cached surface, keeps the toggle row stationary, and preserves the displayed height when reversed.

The Steady Spark logo also appears beside the desktop wordmark, in the window and tray icons, in the Windows launcher/runtime file icons, and on the phone page and home-screen shortcut. Its editable vector sources are `assets/logo.svg` and `assets/app-icon.svg`.

See [testing.md](testing.md) for optional rendered QR decoding checks.

The phone page edits the same in-memory profiles and global settings as the desktop, including favorites and both weapon slots; a favorites strip at the top switches operators in one tap, and the operator picker groups Favorites above All operators. Changes apply immediately; use **Save changes** to persist them. It refreshes desktop changes while you are not editing an input. It embeds its display font and has no external fonts, libraries, or internet dependency. Keep the PC app running. If Windows requests network permission, allow the app on the private network used by the phone.

Phone access can be disabled in Settings. Pairing codes and tokens change when the app restarts. API calls require the session token, pairing attempts are rate-limited, and foreign origins/hosts are rejected. The app does not configure router forwarding or expose a public service. This HTTP control page is intended for a trusted local network.

Pairing throttling uses a bounded, expiring per-IP table and a monotonic 60-second window. It does not block correctly authenticated requests. Pairing codes and bearer tokens use a fixed-work comparison for equal-length values. The phone page follows the selected desktop language (English, Portuguese, French, Spanish, German, Arabic, Hebrew, Chinese, Japanese) and retains its token only in browser session storage.

The page includes a manifest, standalone-mode hints, and locally generated home-screen icons. Use your browser's Add to Home Screen action where available. Full PWA installation/promotion depends on the browser and its HTTPS requirements; a LAN HTTP address cannot guarantee it. The PC app and local network are still required, even when opening a home-screen shortcut.

The sidebar, operator controls, and Settings use fading scrollbars. They start invisible, fade in when scrolling, and fade out after three seconds without scrolling. Dragging keeps them visible until three seconds after release. Their narrow lanes remain reserved so fades never move the content.

The Save icon beside the gear stays neutral ink when saved and uses the current accent when settings are unsaved. It stays flat with the same artwork and transparent background, without a glow or pulse. Failed saves retain the unsaved accent and expose the error in the tooltip; successful saves restore neutral ink. The main window has no footer strip. The header gear opens an inline Settings page for hotkeys, language, sound, and the quick guide. The back arrow, gear, or an operator selection returns to the operator page. The open / hide window binding changes the opening shortcut. The header shows only Recoil off / on; its toggle hotkey remains configurable in Settings. Changing operators preserves unsaved profile edits in memory. Hotkey assignments save immediately, matching the recovered behavior.

Settings use `r6_config.bin` in the launcher directory. Existing valid files remain compatible. Legacy accent and radius values are normalized on load to the fixed GoofyUI purple and the standard background radius, and the next save writes the normalized values. Optional `titio.png` can be placed in the same directory.

Every operator (72 in total, Ace through Noor) has a real embedded SVG icon, without generated tiles or initials placeholders, and supports an optional secondary weapon profile. Most icons come from the supplied operator folder; Ace uses its real icon from [r6operators](https://github.com/marcopixel/r6operators/tree/master/operators/ace) and Noor (the Year 11 Season 3 defender) the one from the maintained [zerobertoo/r6operators](https://github.com/zerobertoo/r6operators/tree/master/operators/noor) fork. The Downloads folder is not needed at runtime. Noor's profile and selection persist in `azec_weapons.json`, so `r6_config.bin` keeps its exact 3,734-byte layout.

Desktop text uses embedded Geist fonts with Fredoka display headings, the bundled Press Start 2P pixel font for the Azec wordmark, and Windows Consolas for connection details. Fonts work without installation or internet access. Font and additional icon licenses are retained in `licenses/Geist`, `licenses/Fredoka`, `licenses/PressStart2P`, and `licenses/r6operators`.

Desktop application strings live in `src/strings.hpp` (lookup and enum), one table per language in `src/lang_*.hpp` (English, Portuguese, French, Spanish, German, Arabic, the Arabic "Free Palestine" prank behind the Hebrew picker entry, Simplified Chinese, Japanese), and bundled flag SVGs selected by `src/flags.hpp`; picking Hebrew shows the Israel flag, then reveals a Free Palestine popup with the Palestine flag and switches every label to Arabic prank text. Portuguese labels for bundled Qt color/file dialogs stay in `strings.hpp`. The phone dictionary in `assets/phone.html` covers the same languages, with RTL layout for Arabic. Favorites now include schema version 1 while still accepting existing unversioned files. Save/server/profile-load diagnostics go to `azec.log`, capped at 256 KiB with one rotated backup. Request bodies and pairing secrets are never logged. Background dots repaint only after cursor or appearance changes; operator pixmaps share a bounded Qt cache.

## Mouse output

Movement uses **Windows SendInput**, with `MOUSEEVENTF_MOVE` and relative integer X/Y deltas. Rapid fire uses the same API with left-button up/down events. The original binary's recovered workers also use SendInput.

The recovered fractional accumulation, shared X/Y humanization jitter, 12/30 ms recoil intervals, and rapid-fire sequence (up → 5 ms → down → 25 ms → up → configured delay clamped to 30-150 ms) remain in the C++ engine. The unused Logitech DLL lookup was removed. Idle recoil checks wait 8 ms while disabled and 4 ms while awaiting a press; idle rapid-fire press checks wait 4 ms. This may add a few milliseconds to first-press detection; active movement/fire timings are unchanged. Real input latency has not been measured in-game.


Country flag artwork is bundled unchanged from [flag-icons v7.3.2](https://github.com/lipis/flag-icons/tree/v7.3.2), with the MIT license in `licenses/flag-icons/`. The language picker preserves the original emblems, stars, and Arabic script rather than approximating them in code.
