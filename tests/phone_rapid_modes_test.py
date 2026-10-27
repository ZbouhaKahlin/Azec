"""Focused headless phone mode choices against an isolated dry-run app."""
import json
from pathlib import Path
import tempfile

from playwright.sync_api import sync_playwright
from run_checks import ROOT, start, stop


def run():
    parent = ROOT / '.build' / 'verification'
    parent.mkdir(parents=True, exist_ok=True)
    folder = Path(tempfile.mkdtemp(prefix='phone-rapid-modes-', dir=parent))
    process = None
    checks = {}
    try:
        process, info = start(folder)
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(channel='chrome', headless=True, args=['--mute-audio'])
            page = browser.new_page(viewport={'width': 390, 'height': 844}, is_mobile=True, has_touch=True)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(info['address'])
            page.locator('#code').fill(info['code'])
            page.get_by_role('button', name='Connect', exact=True).click()
            page.wait_for_selector('#controls:not([hidden])')
            page.locator('#phoneSettings > summary').click()
            page.locator('#rapidModePicker').click()
            checks['exactly_two_mode_choices'] = page.locator('#pickerOptions button').count() == 2
            page.get_by_role('option', name='Hold', exact=True).click()
            page.wait_for_function('()=>state.globals.rapidMode===1&&state.dirty&&pending===0')
            checks['hold_choice_reaches_shared_state'] = page.locator('#rapidModePicker').inner_text() == 'Hold'
            checks['choice_is_pending_before_save'] = not (folder / 'azec_weapons.json').exists()
            page.locator('#save').click()
            page.wait_for_function('()=>!state.dirty&&pending===0')
            checks['save_persists_hold'] = json.loads((folder / 'azec_weapons.json').read_text())['rapidMode'] == 1
            page.locator('#rapidModePicker').click()
            page.get_by_role('option', name='Double click', exact=True).click()
            page.wait_for_function('()=>state.globals.rapidMode===0&&state.dirty&&pending===0')
            checks['double_click_choice_reaches_shared_state'] = page.locator('#rapidModePicker').inner_text() == 'Double click'
            checks['new_pending_choice_keeps_saved_hold'] = json.loads((folder / 'azec_weapons.json').read_text())['rapidMode'] == 1
            checks['phone_has_no_script_errors'] = not errors
            browser.close()
    finally:
        stop(process)
        (folder / 'phone-info.json').unlink(missing_ok=True)
    report = {'passed': all(checks.values()), 'checks': checks}
    (folder / 'phone-rapid-modes.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))
    if not report['passed']:
        raise RuntimeError('Phone rapid-mode checks failed')


if __name__ == '__main__':
    run()
