"""Optional real-browser phone checks (requires Playwright and installed Chrome)."""
import json
from pathlib import Path
import tempfile

from playwright.sync_api import sync_playwright
from run_checks import ROOT, start, stop


def run():
    folder = Path(tempfile.mkdtemp(prefix='phone-browser-', dir=ROOT / '.build' / 'verification'))
    process = None
    checks = {}
    try:
        process, info = start(folder)
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(channel='chrome', headless=True)
            context = browser.new_context(viewport={'width': 390, 'height': 844}, locale='pt-BR', is_mobile=True, has_touch=True)
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(info['address'])
            checks['unpaired_page_defaults_to_english'] = page.locator('#pair h1').inner_text() == 'Connect to your PC'
            page.locator('#code').fill(info['code'])
            page.get_by_role('button', name='Connect', exact=True).click()
            page.wait_for_selector('#controls:not([hidden])')
            checks['pairing_opens_controls_in_english'] = page.locator('html').get_attribute('lang') == 'en'
            checks['embedded_font_loads'] = page.evaluate("document.fonts.ready.then(()=>document.fonts.check('16px Fredoka'))")
            checks['nine_language_options'] = page.locator('#language option').count() == 9
            page.locator('summary').click()
            page.locator('#languagePicker').click()
            page.locator('#pickerOptions button').filter(has_text='Português').click()
            page.wait_for_function("()=>document.documentElement.lang==='pt-BR' && pending===0")
            checks['portuguese_matches_desktop_id'] = page.evaluate('state.globals.language===0')
            checks['function_key_names_are_not_translated_as_errors'] = page.evaluate("t('F1')==='F1' && t('Tab')==='Tab'")
            page.locator('#key4Picker').click()
            page.locator('#pickerSearch').fill('F7')
            page.get_by_role('option', name='F7', exact=True).click()
            page.wait_for_function('()=>state.keys[4].value===118 && pending===0')
            checks['hotkey_picker_updates_requested_action'] = page.locator('#key4Picker span').inner_text() == 'F7'
            page.locator('#key4Picker').click()
            page.locator('#pickerSearch').fill('Insert')
            page.get_by_role('option', name='Insert', exact=True).click()
            page.wait_for_function('()=>pending===0')
            checks['collision_restores_binding_without_disconnect'] = page.evaluate("state.keys[4].value===118 && document.getElementById('connection').classList.contains('connected') && document.getElementById('notice').classList.contains('error')")
            page.locator('#key4Picker').click()
            page.locator('#pickerSearch').fill('F1')
            page.screenshot(path=str(folder / 'phone-hotkeys.png'))
            page.keyboard.press('Escape')
            checks['picker_escape_restores_focus'] = page.evaluate("!document.getElementById('picker').open && document.activeElement.id==='key4Picker'")
            page.locator('#languagePicker').click()
            page.get_by_role('option', name='🇺🇸 English (US)', exact=True).click()
            page.wait_for_function("()=>document.documentElement.lang==='en' && pending===0")
            checks['english_matches_desktop_id'] = page.evaluate('state.globals.language===1')
            page.locator('#operatorPicker').click()
            page.locator('#pickerSearch').fill('Ash')
            page.get_by_role('option', name='Ash', exact=True).click()
            page.wait_for_function('()=>state.operatorId===4 && pending===0')
            checks['operator_picker_selects_profile'] = page.locator('#operatorName').inner_text() == 'Ash'
            layout_ok = True
            dialogs_fit = True
            for width in (320, 390, 430, 768):
                page.set_viewport_size({'width': width, 'height': 844})
                for language in range(9):
                    page.evaluate('(language)=>mutate({action:"setGlobal",values:{language}})', language)
                    layout_ok &= page.evaluate('document.documentElement.scrollWidth<=innerWidth')
                    page.locator('#languagePicker').click()
                    dialogs_fit &= page.evaluate("(()=>{const r=document.getElementById('picker').getBoundingClientRect();return r.left>=0&&r.right<=innerWidth&&r.top>=0&&r.bottom<=innerHeight})()")
                    page.keyboard.press('Escape')
            checks['all_languages_fit_four_mobile_widths'] = layout_ok
            checks['language_dialog_fits_four_widths'] = dialogs_fit
            page.set_viewport_size({'width': 390, 'height': 844})
            page.evaluate('()=>mutate({action:"setGlobal",values:{language:1}})')
            page.locator('summary').scroll_into_view_if_needed()
            page.screenshot(path=str(folder / 'phone-settings.png'))
            page.locator('#languagePicker').click()
            page.screenshot(path=str(folder / 'phone-languages.png'))
            page.keyboard.press('Escape')
            page.evaluate('()=>mutate({action:"save"})')
            checks['custom_hotkey_persists'] = json.loads((folder / 'azec_weapons.json').read_text())['primaryKey'] == 118
            page.evaluate("()=>{token='expired-session';return mutate({action:'save'})}")
            checks['expired_session_returns_to_pairing'] = page.locator('#pair').is_visible() and page.locator('#controls').is_hidden()
            qr_context = browser.new_context(viewport={'width': 390, 'height': 844}, is_mobile=True, has_touch=True)
            qr_page = qr_context.new_page()
            qr_page.goto(info['address'] + '/#token=' + info['token'])
            qr_page.wait_for_selector('#controls:not([hidden])')
            checks['qr_link_opens_controls_without_code'] = qr_page.locator('#pair').is_hidden()
            qr_context.close()
            checks['no_javascript_errors'] = not errors
            browser.close()
        report = {'passed': all(checks.values()), 'checks': checks, 'errors': errors}
        (folder / 'browser-verification.json').write_text(json.dumps(report, indent=2))
        print(json.dumps(report, indent=2))
        print('Browser artifacts: ' + str(folder))
        return report['passed']
    finally:
        stop(process)
        (folder / 'phone-info.json').unlink(missing_ok=True)


if __name__ == '__main__':
    raise SystemExit(0 if run() else 1)
