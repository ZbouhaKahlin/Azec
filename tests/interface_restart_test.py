"""Exercise the Settings apply button through a real, isolated app restart."""
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import time
import urllib.request

from run_checks import APP, ROOT, start, stop


def wait_for_info(path, predicate, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            info = json.loads(path.read_text())
            if predicate(info):
                return info
        except (OSError, ValueError):
            pass
        time.sleep(.03)
    raise TimeoutError('Restart did not produce the expected runtime')


def wait_for_exit(pid):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_uint32]
    kernel.OpenProcess.restype = ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    kernel.CloseHandle.argtypes = [ctypes.c_void_p]
    handle = kernel.OpenProcess(0x100000, False, pid)
    if handle:
        try:
            if kernel.WaitForSingleObject(handle, 12000) != 0:
                subprocess.run(['taskkill', '/PID', str(pid), '/F'], capture_output=True)
                raise TimeoutError('Restarted dry-run process did not exit')
        finally:
            kernel.CloseHandle(handle)


def run(parent):
    folder = Path(tempfile.mkdtemp(prefix='interface-restart-', dir=parent))
    process = None
    checks = {}
    child = None
    try:
        process, info = start(folder)

        def api(action):
            request = urllib.request.Request('http://127.0.0.1:' + str(info['port']) + '/api/update',
                data=json.dumps(action).encode(), headers={'Content-Type': 'application/json',
                'Authorization': 'Bearer ' + info['token']})
            with urllib.request.urlopen(request, timeout=5) as response:
                return json.load(response)

        api({'action': 'selectOperator', 'operator': 4})
        api({'action': 'setProfile', 'operator': 4, 'slot': 0, 'values': {'vertical': 12.3}})
        api({'action': 'setDual', 'operator': 4, 'enabled': True})
        api({'action': 'setProfile', 'operator': 4, 'slot': 1, 'values': {'vertical': 23.7}})
        api({'action': 'setFavorite', 'operator': 4, 'enabled': True})
        api({'action': 'selectOperator', 'operator': 71})
        api({'action': 'setProfile', 'operator': 71, 'slot': 0, 'values': {'vertical': 31.5}})
        api({'action': 'save'})
        checks['fresh_app_uses_goofy'] = info['interface'] == 'goofy'
        stop(process)
        process = None
        binary_hash = hashlib.sha256((folder / 'r6_config.bin').read_bytes()).hexdigest()
        weapon_bytes = (folder / 'azec_weapons.json').read_bytes()
        for target in ('rally', 'goofy'):
            path = folder / 'restart-info.json'
            path.unlink(missing_ok=True)
            process = subprocess.Popen([str(ROOT / 'runtime' / 'Azec-checks.exe'), '--dry-run',
                '--restart-target=' + target, '--phone-info=restart-info.json', '--exit-after-ms=1800'],
                cwd=folder, creationflags=subprocess.CREATE_NO_WINDOW)
            parent_pid = process.pid
            child = wait_for_info(path, lambda item: item['pid'] != parent_pid and item['interface'] == target)
            process.wait(timeout=10)
            checks[target + '_restarts_into_selected_interface'] = process.returncode == 0 and child['dryRun']
            checks[target + '_keeps_config_directory'] = json.loads((folder / 'azec_ui.json').read_text())['interface'] == target
            checks[target + '_preserves_binary_profiles'] = hashlib.sha256((folder / 'r6_config.bin').read_bytes()).hexdigest() == binary_hash
            checks[target + '_preserves_secondary_and_extension_profiles'] = (folder / 'azec_weapons.json').read_bytes() == weapon_bytes
            checks[target + '_preserves_favorites'] = json.loads((folder / 'azec_ui.json').read_text())['favorites'] == ['Ash']
            request = urllib.request.Request('http://127.0.0.1:' + str(child['port']) + '/api/state',
                headers={'Authorization': 'Bearer ' + child['token']})
            with urllib.request.urlopen(request, timeout=5) as response:
                state = json.load(response)
            checks[target + '_shared_backend_restores_selection'] = state['operatorId'] == 71 and state['profile']['vertical'] == 31.5
            wait_for_exit(child['pid'])
            child = None
            process = None
        report = {'passed': all(checks.values()), 'checks': checks}
        (folder / 'interface-restart-verification.json').write_text(json.dumps(report, indent=2))
        if not report['passed']:
            raise AssertionError([name for name, passed in checks.items() if not passed])
        return report
    finally:
        stop(process)
        if child:
            wait_for_exit(child['pid'])
        (folder / 'phone-info.json').unlink(missing_ok=True)
        (folder / 'restart-info.json').unlink(missing_ok=True)


if __name__ == '__main__':
    parent = ROOT / '.build' / 'verification'
    parent.mkdir(parents=True, exist_ok=True)
    print(json.dumps(run(parent), indent=2))
