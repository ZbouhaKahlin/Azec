"""Decode the production QR captures from one or more --ui-self-test directories.

Requires OpenCV for Python (python -m pip install opencv-python).
Session links are compared by SHA-256 and never printed or saved in this report.
"""
import hashlib
import json
from pathlib import Path
import sys

import cv2


def verify(folder):
    cases = json.loads((folder / 'qr-scan-cases.json').read_text())
    checks = {}
    for case in cases:
        path = folder / case['file']
        image = cv2.imread(str(path))
        if image is None:
            raise RuntimeError('Missing QR capture: ' + str(path))
        value, _, _ = cv2.QRCodeDetector().detectAndDecode(image)
        checks[case['file']] = bool(value) and hashlib.sha256(value.encode()).hexdigest() == case['sha256']
    report = {'passed': all(checks.values()), 'checks': checks}
    (folder / 'qr-scan-verification.json').write_text(json.dumps(report, indent=2))
    print(folder.name + ': ' + json.dumps(report))
    return report['passed']


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('Usage: python tests/qr_scan.py <UI fixture directory> [...]')
    results = [verify(Path(folder)) for folder in sys.argv[1:]]
    raise SystemExit(0 if all(results) else 1)
