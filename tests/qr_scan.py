"""Decode the production QR captures from one or more --ui-self-test directories.

Requires OpenCV for Python (python -m pip install opencv-python).
Session links are compared by SHA-256 and never printed or saved in this report.
"""
import hashlib
import json
from pathlib import Path
import sys

import cv2
import numpy as np


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
    checks = {}
    for case in cases:
        path = folder / case['file']
        image = cv2.imread(str(path))
        if image is None:
            raise RuntimeError('Missing QR capture: ' + str(path))
        for variant, rendered in scan_variants(image):
            value, _, _ = cv2.QRCodeDetector().detectAndDecode(rendered)
            checks[case['file'] + ':' + variant] = bool(value) and hashlib.sha256(value.encode()).hexdigest() == case['sha256']
    report = {'passed': all(checks.values()), 'checks': checks}
    (folder / 'qr-scan-verification.json').write_text(json.dumps(report, indent=2))
    print(folder.name + ': ' + json.dumps(report))
    return report['passed']


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('Usage: python tests/qr_scan.py <UI fixture directory> [...]')
    results = [verify(Path(folder)) for folder in sys.argv[1:]]
    raise SystemExit(0 if all(results) else 1)
