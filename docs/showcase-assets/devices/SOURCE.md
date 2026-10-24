# iPhone presentation frame

- Upstream: [weirdapps/mockups](https://github.com/weirdapps/mockups).
- Pinned revision: `364a4794785ce8cd615c31c0fa21265973c6496d`.
- Original asset: [`16 Pro Max - Black Titanium.png`](https://github.com/weirdapps/mockups/blob/364a4794785ce8cd615c31c0fa21265973c6496d/src/mockups/frames/16%20Pro%20Max%20-%20Black%20Titanium.png), copied without modification as `iphone-16-pro-max-black.png` (1520 × 3068).
- License: MIT, copyright 2026 Dimitrios Plessas. The full notice is in [licenses/iphone-mockup-MIT.txt](../../../licenses/iphone-mockup-MIT.txt).
- Upstream notes that the frame is based on Apple's publicly available product imagery. iPhone is a trademark of Apple Inc. Neither upstream nor Azec is affiliated with or endorsed by Apple.

`iphone-16-pro-max-screen-mask.png` is a derived alpha mask: a four-neighbor flood fill of the original frame's inner transparent region, seeded at its center, accepting alpha below 50. This excludes transparent pixels outside the device and preserves the original screen curves and camera cutouts. The mask was generated with the browser Canvas API; the hardware overlay remains the original asset.

The original screen rectangle is x=100, y=100, width=1320, height=2868. The HTML presentation uses a 440 × 956 coordinate system scaled uniformly by three, with a 62 px status area, an 856 px app capture, and a 38 px home area. App captures are 880 × 1712 browser screenshots of an isolated dry-run instance, displayed at 440 × 856 without stretching or cropping. They are browser captures, not physical-iPhone captures.

The latest README image is `docs/showcase-latest.png`, composed from `docs/showcase-latest.html`. Its `latest-charcoal.png` desktop and phone captures were made from the working tree on 2026-10-24 with isolated sample settings. The native desktop render uses the current app source and freshly built objects with a temporary capture entry point; the phone browser connects to that same dry-run instance. The browser viewport is 440 × 856 at 2× scale. No personal profiles or pairing credentials appear in the image.

Earlier `docs/showcase-assets/desktop/*.png` images came from the desktop verification fixture; earlier phone captures also use a 440 × 856 viewport at 2× scale. These remain palette examples.

All three compositions use an 1800 × 1100 canvas with local assets and a lilac grid background. The latest image pairs one desktop with one uniformly scaled 365 px phone frame. The earlier `docs/showcase.html` and `docs/showcase-pastel.html` compositions retain their cascades and 320 px phone frames. Export with local file access enabled (`--allow-file-access-from-files`) so the phone screen mask loads.
