"""Decode the production QR captures from one or more --ui-self-test directories.

Requires OpenCV for Python and Node.js for the bundled phone-decoder fallback.
Session links are compared by SHA-256 and never printed or saved in this report.
"""
import hashlib
import json
import base64
from pathlib import Path
import subprocess
import sys

import cv2
import numpy as np


def phone_decoder(cases):
    """Check OpenCV misses with the bundled phone decoder; return hashes only."""
    decoder = Path(__file__).resolve().parents[1] / 'third_party/jsqr/jsQR.js'
    script = """
const fs=require('node:fs'),crypto=require('node:crypto'),jsQR=require(process.argv[1]);
const cases=JSON.parse(fs.readFileSync(0,'utf8')),out={};
for(const item of cases){
    const pixels=new Uint8ClampedArray(Buffer.from(item.pixels,'base64'));
    const result=jsQR(pixels,item.width,item.height);
    out[item.name]=result?crypto.createHash('sha256').update(result.data).digest('hex'):null;
}
process.stdout.write(JSON.stringify(out));
"""
    result = subprocess.run(['node', '-e', script, str(decoder)], input=json.dumps(cases),
                            capture_output=True, text=True, check=True, timeout=30,
                            creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == 'win32' else 0)
    return json.loads(result.stdout)


def scan_variants(image):
    """Small camera-like degradations, not a substitute for a physical phone."""
    yield 'original', image
    yield 'soft-focus', cv2.GaussianBlur(image, (3, 3), .5)
    yield 'lower-contrast', cv2.convertScaleAbs(image, alpha=.65, beta=70)
    yield 'smaller', cv2.resize(image, None, fx=.8, fy=.8, interpolation=cv2.INTER_AREA)
    height, width = image.shape[:2]
    source = np.float32([[0, 0], [width - 1, 0], [width - 1, height - 1], [0, height - 1]])
    target = np.float32([[8, 4], [width - 5, 0], [width - 1, height - 5], [0, height - 1]])
    transform = cv2.getPerspectiveTransform(source, target)
    yield 'angled', cv2.warpPerspective(image, transform, (width, height), borderValue=(255, 255, 255))


def verify(folder):
    cases = json.loads((folder / 'qr-scan-cases.json').read_text())
    checks, decoders, opencv_misses, retry = {}, {}, [], []
    for case in cases:
        path = folder / case['file']
        image = cv2.imread(str(path))
        if image is None:
            raise RuntimeError('Missing QR capture: ' + str(path))
        for variant, rendered in scan_variants(image):
            value, _, _ = cv2.QRCodeDetector().detectAndDecode(rendered)
            name = case['file'] + ':' + variant
            checks[name] = bool(value) and hashlib.sha256(value.encode()).hexdigest() == case['sha256']
            if checks[name]:
                decoders[name] = 'OpenCV'
            elif not value:
                # Some valid QR masks are missed by OpenCV at particular raster
                # sizes. Require the actual phone decoder to read the exact
                # failed image, without rescaling or replacing the capture.
                opencv_misses.append(name)
                pixels = cv2.cvtColor(rendered, cv2.COLOR_BGR2RGBA)
                retry.append({'name': name, 'width': pixels.shape[1], 'height': pixels.shape[0],
                              'pixels': base64.b64encode(pixels.tobytes()).decode(), 'expected': case['sha256']})
    if retry:
        decoded = phone_decoder(retry)
        for case in retry:
            checks[case['name']] = decoded.get(case['name']) == case['expected']
            decoders[case['name']] = 'bundled jsQR'
    report = {'passed': all(checks.values()), 'checks': checks, 'decoders': decoders, 'opencv_misses': opencv_misses}
    (folder / 'qr-scan-verification.json').write_text(json.dumps(report, indent=2))
    print(folder.name + ': ' + json.dumps(report))
    return report['passed']


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('Usage: python tests/qr_scan.py <UI fixture directory> [...]')
    results = [verify(Path(folder)) for folder in sys.argv[1:]]
    raise SystemExit(0 if all(results) else 1)
