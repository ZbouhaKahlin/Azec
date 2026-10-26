# Screenshots and the approved Azec showcase

Read this guide before taking Azec screenshots, refreshing the README picture, or creating desktop-and-phone background variations. It records the composition approved on 2026-10-26. Capture the latest real interfaces while preserving this layout unless the user requests a different one.

## Approved reference

- Main picture: [showcase-11J.png](showcase-11J.png), **1920 × 1080**, used by [README.md](../README.md).
- Approved alignment is present in source commit `1d6f7dd76370818a3a10339189a973b12f87c1d8`.
- The user explicitly approved this version after the final logo adjustment. Preserve the A-shaped mark's exact position below; do not realign it from scratch.
- Use real screenshots as image layers. The optional AI-finished version is not the approved reference; do not regenerate UI labels, controls, operator icons, or background artwork with an image model.
- This is a promotional-image recipe. It does not authorize changing the application's layout, opacity, theme, profiles, or behavior.

## Assets and where to find them

Paths in this table are relative to the Azec repository root unless absolute.

| Asset | Location |
| --- | --- |
| Approved composite | `docs/showcase-11J.png` |
| A-shaped logo | `assets/logo.svg` — original 64 × 64 SVG view box, fill `#EBEBEB` |
| Phone hardware frame | `docs/showcase-assets/devices/iphone-16-pro-max-black.png` — 1520 × 3068 |
| Phone screen mask | `docs/showcase-assets/devices/iphone-16-pro-max-screen-mask.png` |
| Frame geometry, attribution, and license | [devices/SOURCE.md](showcase-assets/devices/SOURCE.md), [MIT notice](../licenses/iphone-mockup-MIT.txt) |
| Local desktop capture | `docs/showcase-assets/current/desktop.png` — approved capture 1296 × 968 |
| Local phone page capture | `docs/showcase-assets/current/phone-screen.png` — 1320 × 2868 |
| Local framed phone | `docs/showcase-assets/current/phone-mockup.png` — 1520 × 3068 |
| Local desktop with presentation alpha adjustment | `docs/showcase-assets/current/desktop-transparent.png` |
| All original background files | `%USERPROFILE%/Desktop/Code-Dev/Canav/backgrounds/` |

The `current/` captures are local reference assets and may be absent from another checkout. Check their existence and freshness; recapture when necessary. The tracked composite, exact recipe here, logo, frame, and mask remain the reference. Never assume that an ignored build folder or a historical screenshot proves the current UI.

### The two Meadows

The approved main picture uses **`%USERPROFILE%/Desktop/Code-Dev/Canav/backgrounds/meadow.png`**: the dark green and olive painted field, without the broad pale sky of the original.

**`meadow.jpeg` is a different picture**, the first/original Meadow: a light purple-blue sky over a green field with flowers. When the user asks for the "first Meadow" or "original Meadow," use this JPEG. Do not silently substitute the PNG.

The background folder contains 16 images: `earth`, `japan`, `lake-como`, `meadow`, `alpine`, `blossom`, `cornflower`, and `drift`, each with both `.jpeg` and `.png` versions. These are distinct artworks, not interchangeable encodings. Inspect the exact requested file. The folder also has packaged copies under Canav's `release/win-unpacked/resources/backgrounds/`; prefer the source `backgrounds/` folder.

Draw the selected background into `(0, 0, 1920, 1080)`, filling that exact rectangle as in the approved composite. Keep this mapping consistent behind the translucent desktop.

## Capture the current interfaces safely

Follow [testing.md](testing.md), including its noninteractive isolation and credential rules. A screenshot request calls for captures, not the full test suite.

1. Inspect the current working tree and rebuild with `build.cmd` if the binary or embedded phone page is stale. Preserve unrelated work. Do not stop the user's running app to deploy a build.
2. Create a **fresh** fixture directory under `.build/verification/`. All example configuration belongs there, never in the repository root, a live portable runtime, or `%LOCALAPPDATA%/Azec`.
3. Start `runtime/Azec.exe` through `tests/hidden_process.py` with `--dry-run`. `tests/run_checks.py` provides `start(folder)` and `stop(process)` for an isolated phone-enabled fixture. Never launch a visible fallback.
4. Seed the example state below through the fixture's existing API and Save. Resolve operator IDs by name from `/api/state`; do not infer IDs from a visual row number. For `setDual` and `setProfile`, include the current `profileSet` identity. Favorites live in the fixture's `azec_ui.json` or the existing favorite API.
5. Stop that fixture, then capture the desktop from the same saved fixture through `HiddenProcess`, passing `--dry-run --capture-ui=<absolute-output-folder>`. Set `QT_SCALE_FACTOR=1.5` and `QT_ENABLE_HIGHDPI_SCALING=1` for this capture. The documentation path captures `desktop.png` and `settings.png`, then exits. Wait for completion and close its process tree in `finally`.
6. Restart an isolated phone-enabled instance against the same fixture. Pair the current embedded page using the fixture's code in headless Chrome, with `--mute-audio`, English locale, mobile/touch emulation, viewport **440 × 956 CSS pixels**, and **device scale factor 3**. This produces a **1320 × 2868** screen capture. The existing workflow is illustrated by `tests/phone_browser_test.py`.
7. Wait until paired controls are visible, the Ash primary state is loaded, pending requests finish, and motion settles. Keep the normal phone layout, Save dock, and scroll position. Capture the viewport, not a full-page image stretched into a phone. Verify the labels, numbers, toggles, and absence of browser errors.
8. Close headless Chrome and every fixture process tree in `finally`. Remove the fixture's temporary `phone-info.json` after use. Do not print codes, tokens, token URLs, or revealed QR images; no pairing credentials belong in any capture, report, or committed asset.

### Matching example state

| Setting | Approved example |
| --- | --- |
| Language / theme | English (`1`) / Dark |
| Favorites | Ace, Ash |
| Active operator / weapon | Ash / Primary |
| Separate secondary profile | Enabled; return to Primary after enabling |
| Vertical / horizontal | `12.3` / `-2.4` |
| Shot delay | `75 ms` |
| Humanize / Rapid fire | Enabled / enabled |
| Recoil | Off on both surfaces |
| Feedback sound | Off in the fixture |
| Save state | Saved, with no pending profile edits |

The phone's Output card contains only Recoil active. Rapid fire active belongs above the lower Rapid fire heading. Refresh captures from the actual UI rather than retouching an obsolete screenshot to resemble this arrangement. State clearly that the phone capture is browser emulation, not physical-phone testing.

## Exact composition

All positions below are **canvas pixels on a 1920 × 1080 output**, measured from the top left. Composite once at this size and scale the whole result for previews.

| Element | X | Y | Width | Height / placement |
| --- | ---: | ---: | ---: | --- |
| Background | 0 | 0 | 1920 | 1080 |
| Desktop | 585 | 110 | 1240 | Preserve source aspect ratio: `1240 * sourceHeight / sourceWidth`; approved capture gives `926.1728395061729` |
| Full phone frame | 140 | 170 | 340 | `340 * 3068 / 1520 = 686.2631578947369` |
| A-shaped logo SVG draw box | 209.55908203125 | **920** | **72** | **72** |
| `zec` text | 281.55908203125 | **976 baseline** | Measured text | Left aligned, alphabetic baseline |
| `DESKTOP + PHONE` | **310 center** | **1021 baseline** | Measured text | Center aligned, alphabetic baseline |

### Wordmark and subtitle

The logo replaces the first letter of **Azec**. Draw the logo plus **`zec`**, not the logo plus another full "Azec". The whole branding unit is centered under the phone at **x = 310**.

- Wordmark text: `300 70px "Segoe UI"`, fill `#EBEBEB`, Canvas `letterSpacing = "10px"`, `textBaseline = "alphabetic"`, left aligned. Do not use a font fallback when reproducing exact geometry.
- Logo: draw the existing SVG, including its original view-box padding, at **72 × 72**, **y = 920**. These are SVG draw-box coordinates, not the visible path's bounding box. Do not move the wordmark or subtitle when adjusting only the mark.
- Compute the group's left edge as `310 - (72 + measureText("zec").width) / 2`, with zero extra gap. With the approved Segoe UI rendering, text width is `128.8818359375`, left edge is `209.55908203125`, and `zec` starts 72 pixels later.
- Subtitle: exactly **`DESKTOP + PHONE`**, `400 17px "Segoe UI"`, fill `#EBEBEB`, `letterSpacing = "4px"`, centered at **(310, 1021)** using an alphabetic baseline. Wait for fonts to load before measuring or drawing.
- The final approved alignment is intentional and optical. Preserve **logo y = 920**, **wordmark baseline = 976**, and **subtitle baseline = 1021** on future refreshes.

### Phone construction

Create a transparent **1520 × 3068** layer. Draw the phone screenshot into **x = 100, y = 100, width = 1320, height = 2868**. Apply the existing screen mask with Canvas `destination-in`, then restore `source-over` and draw the unmodified hardware frame on top. Preserve the screen curves, frame, camera cutout, and original aspect ratio. The phone body and screen stay opaque in the promotional composite.

### Desktop transparency and shadows

Start from the native PNG with its existing alpha; do not flatten it onto a black background first. For each pixel with **0 < alpha < 245**, replace alpha with **round(alpha × 0.8)**. Leave the RGB channels and other alpha values unchanged, including fully opaque text/control pixels. This is the approved interpretation of "20% more transparent," not a blanket 80% opacity on the complete screenshot and not an app-setting change. Always derive it from the raw capture; do not apply the multiplier a second time to an already adjusted image.

Before drawing that layer, clip the desktop's exact rectangle with a **24 px** rounded corner radius and redraw the same full-canvas background through a **13 px blur**. Restore the clip/filter, then draw the adjusted screenshot at the desktop coordinates.

The desktop layer uses shadow color `rgba(0,0,0,.22)`, blur **22 px**, and vertical offset **12 px**. The phone uses `rgba(0,0,0,.32)`, blur **16 px**, and vertical offset **12 px**. Both horizontal shadow offsets are zero. Keep these effects confined to the respective layers.

## Review, previews, and delivery

- Inspect the actual exported PNG, including a close-up of the branding. Verify the real UI, both surfaces' matching values, exact background filename, sharp text, no duplicate name, no stretched screen, and the positions above. A successful render is not visual approval.
- For a logo-only correction, compare pixels and confirm changes are confined to the A-shaped mark. Keep every other element in place.
- Show the result when the user requests it; include the full image and a readable branding close-up. For HTML comparison pages, check with `html_preview` and show with `html_render` when those tools are available. Keep any displayed file path separate from an image URL that an inliner may replace with a data URI.
- Earth, **original Meadow**, and Drift preview backgrounds use `earth.jpeg`, **`meadow.jpeg`**, and `drift.jpeg`. Put requested alternatives under an ignored `.build/verification/` folder. Keep the approved main background until another is selected. Do not commit or push previews merely because the main picture is approved.
- Only a request to push authorizes committing/pushing the requested picture or documentation. Stage specific paths, preserve unrelated files, and verify the remote commit. This is source-only delivery; no installer build, version stamp, or release is implied.
- The original local helper scripts are `.build/verification/showcase-refresh-20261026/capture_current.py` and `prepare_composition.py`; background previews are under `.build/verification/showcase-background-options-20261026/`. They are optional local conveniences, not durable dependencies. This guide is the reconstruction recipe if those ignored folders disappear. Create a fresh fixture for a new capture rather than reusing the old script's fixed fixture blindly.
