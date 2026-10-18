"""Keep profiles and appearance preferences across a Goofy-only upgrade and restart."""
import hashlib
import json
from pathlib import Path
import tempfile
import urllib.request

from run_checks import ROOT, start, stop


def run(parent):
    folder = Path(tempfile.mkdtemp(prefix='preferences-restart-', dir=parent))
    process = None
    checks = {}
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
        previous_token = info['token']
        stop(process)
        process = None
        binary_hash = hashlib.sha256((folder / 'r6_config.bin').read_bytes()).hexdigest()
        weapon_bytes = (folder / 'azec_weapons.json').read_bytes()
        ui_path = folder / 'azec_ui.json'
        ui = json.loads(ui_path.read_text())
        # A saved choice from an older build must never activate the removed UI.
        ui.update(interface='rally', colorTheme='deep-sea', sidebarCollapsed=True)
        ui_path.write_text(json.dumps(ui))
        for phase in ('upgrade', 'restart'):
            process, info = start(folder)
            checks[phase + '_uses_goofy_in_dry_run'] = info['interface'] == 'goofy' and info['dryRun']
            checks[phase + '_rotates_pairing_session'] = info['token'] != previous_token
            previous_token = info['token']
            checks[phase + '_preserves_binary_profiles'] = hashlib.sha256((folder / 'r6_config.bin').read_bytes()).hexdigest() == binary_hash
            checks[phase + '_preserves_secondary_and_extension_profiles'] = (folder / 'azec_weapons.json').read_bytes() == weapon_bytes
            state = api({'action': 'setFavorite', 'operator': 4, 'enabled': True})
            checks[phase + '_restores_extension_selection'] = state['operatorId'] == 71 and state['profile']['vertical'] == 31.5
            restored_ui = json.loads(ui_path.read_text())
            checks[phase + '_preserves_preferences'] = restored_ui['favorites'] == ['Ash'] and restored_ui['colorTheme'] == 'deep-sea' and restored_ui['sidebarCollapsed'] is True
            checks[phase + '_drops_obsolete_interface_choice'] = 'interface' not in restored_ui
            stop(process)
            process = None
        report = {'passed': all(checks.values()), 'checks': checks}
        (folder / 'preferences-restart-verification.json').write_text(json.dumps(report, indent=2))
        if not report['passed']:
            raise AssertionError([name for name, passed in checks.items() if not passed])
        return report
    finally:
        stop(process)
        (folder / 'phone-info.json').unlink(missing_ok=True)


if __name__ == '__main__':
    parent = ROOT / '.build' / 'verification'
    parent.mkdir(parents=True, exist_ok=True)
    print(json.dumps(run(parent), indent=2))
