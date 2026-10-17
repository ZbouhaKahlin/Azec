"""Isolated HTTP/restart checks; --ui additionally verifies both display scales."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / 'runtime' / 'Azec.exe'

def start(folder):
    process = subprocess.Popen([str(APP), '--dry-run', '--phone-info=phone-info.json'], cwd=folder,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        try:
            info = json.loads((folder / 'phone-info.json').read_text())
            if info.get('pid') == process.pid:
                return process, info
        except (OSError, ValueError):
            pass
        if process.poll() is not None:
            raise RuntimeError('Dry-run instance exited before startup completed')
        time.sleep(.05)
    process.terminate()
    process.wait(timeout=5)
    raise TimeoutError('Dry-run app did not write pairing info within 20 seconds')

def stop(process):
    if process and process.poll() is None:
        process.terminate()
        process.wait(timeout=10)

def run_http(parent):
    folder = Path(tempfile.mkdtemp(prefix='http-', dir=parent))
    process = None
    try:
        process, info = start(folder)
        subprocess.run([sys.executable, str(ROOT / 'tests' / 'phone_test.py'), str(folder)], check=True)
        report = json.loads((folder / 'http-verification.json').read_text())
        if not report['passed']:
            raise RuntimeError('HTTP checks reported a failure')
        duplicate = subprocess.run([str(APP), '--dry-run', '--phone-info=duplicate-info.json'],
                                   cwd=folder, timeout=10, creationflags=subprocess.CREATE_NO_WINDOW)
        if duplicate.returncode != 0 or (folder / 'duplicate-info.json').exists():
            raise RuntimeError('Second instance was not rejected')
        old_token = info['token']
        stop(process)
        (folder / 'phone-info.json').unlink()
        process, info = start(folder)
        import urllib.request
        request = urllib.request.Request('http://127.0.0.1:' + str(info['port']) + '/api/state',
                                         headers={'Authorization': 'Bearer ' + info['token']})
        with urllib.request.urlopen(request, timeout=5) as response:
            state = json.load(response)
        ash = next(op for op in state['operators'] if op['name'] == 'Ash')
        if not ash['dual'] or not ash['favorite'] or old_token == info['token']:
            raise RuntimeError('Restart did not preserve profiles/favorites and rotate session token')
        if state['operatorId'] != 71 or abs(state['profile']['vertical'] - 31.5) > .001:
            raise RuntimeError('Restart lost the extension operator selection or profile')
        weapons = json.loads((folder / 'azec_weapons.json').read_text())
        if abs(weapons['operators']['Ash']['vertical'] - 23.7) > .001:
            raise RuntimeError('Restart lost the secondary profile')
        if weapons.get('selected') != 71 or abs(weapons['primaryProfiles']['Noor']['vertical'] - 31.5) > .001:
            raise RuntimeError('Restart lost the extension profile store')
        (folder / 'restart-verification.json').write_text(json.dumps({'passed': True,
            'checks': {'secondary': True, 'favorites': True, 'session_rotated': True, 'single_instance': True,
                       'extension_operator': True}}, indent=2))
    finally:
        stop(process)
        # Session credentials must never be uploaded as verification artifacts.
        (folder / 'phone-info.json').unlink(missing_ok=True)

def run(ui=False,ui_only=False,qr_scan=False):
    parent = ROOT / '.build' / 'verification'
    parent.mkdir(parents=True, exist_ok=True)
    if not ui_only:
        run_http(parent)
        from recovery_test import run as run_recovery
        run_recovery(parent)
        from interface_restart_test import run as run_interfaces
        run_interfaces(parent)
    if ui:
        for interface in ('goofy', 'rally'):
            for scale in ('1', '1.5'):
                fixture = Path(tempfile.mkdtemp(prefix='ui-' + interface + '-' + scale + '-', dir=parent))
                env = os.environ.copy()
                env['QT_SCALE_FACTOR'] = scale
                subprocess.run([str(ROOT / 'runtime' / 'Azec-checks.exe'), '--ui-self-test', '--interface='+interface], cwd=fixture, env=env, check=True, timeout=150)
                report = json.loads((fixture / 'ui-verification.json').read_text())
                if not report['passed']:
                    raise RuntimeError('UI report failed at scale ' + scale)
                if qr_scan:
                    from qr_scan import verify
                    if not verify(fixture):
                        raise RuntimeError('QR decoding failed at scale ' + scale)
    print('Verification artifacts: ' + str(parent))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--ui', action='store_true')
    parser.add_argument('--ui-only', action='store_true')
    parser.add_argument('--qr-scan', action='store_true', help='Decode UI QR captures (requires opencv-python-headless)')
    args=parser.parse_args()
    run(args.ui or args.ui_only or args.qr_scan,args.ui_only,args.qr_scan)
