# iPhone presentation frame

- Upstream: [weirdapps/mockups](https://github.com/weirdapps/mockups).
- Pinned revision: `364a4794785ce8cd615c31c0fa21265973c6496d`.
- Original asset: [`16 Pro Max - Black Titanium.png`](https://github.com/weirdapps/mockups/blob/364a4794785ce8cd615c31c0fa21265973c6496d/src/mockups/frames/16%20Pro%20Max%20-%20Black%20Titanium.png), copied without modification as `iphone-16-pro-max-black.png` (1520 × 3068).
- License: MIT, copyright 2026 Dimitrios Plessas. The full notice is in [licenses/iphone-mockup-MIT.txt](../../../licenses/iphone-mockup-MIT.txt).
- Upstream notes that the frame is based on Apple's publicly available product imagery. iPhone is a trademark of Apple Inc. Neither upstream nor Azec is affiliated with or endorsed by Apple.

`iphone-16-pro-max-screen-mask.png` is a derived alpha mask: a four-neighbor flood fill of the original frame's inner transparent region, seeded at its center, accepting alpha below 50. This excludes transparent pixels outside the device and preserves the original screen curves and camera cutouts. The mask was generated with the browser Canvas API; the hardware overlay remains the original asset.

The original screen rectangle is x=100, y=100, width=1320, height=2868. The HTML presentation uses a 440 × 956 coordinate system scaled uniformly by three, with a 62 px status area, an 856 px app capture, and a 38 px home area. App captures are 880 × 1712 browser screenshots of an isolated dry-run instance, displayed at 440 × 856 without stretching or cropping. They are browser captures, not physical-iPhone captures.

`docs/showcase.html` (A6, original and dark themes) and `docs/showcase-pastel.html` (C6, pastel themes) are the editable compositions shown in the README. Both use a lilac grid background, two overlapping iPhones in a cascade on the left, and the desktop cascade on the right. The phone frames scale uniformly to 320 px wide. Open the compositions in a browser with an 1800 × 1100 viewport to export the matching PNG. All assets load locally; no network access is needed.
