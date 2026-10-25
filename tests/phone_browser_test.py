"""Optional real-browser phone checks (requires Playwright and installed Chrome)."""
import argparse
import json
from pathlib import Path
import tempfile
from urllib.request import Request, urlopen

from playwright.sync_api import sync_playwright
from run_checks import ROOT, start, stop


def run(full=False):
    (ROOT / '.build' / 'verification').mkdir(parents=True, exist_ok=True)
    folder = Path(tempfile.mkdtemp(prefix='phone-browser-goofy-', dir=ROOT / '.build' / 'verification'))
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
            checks['page_uses_system_fonts'] = page.evaluate("getComputedStyle(document.body).fontFamily").lower().find('system-ui') >= 0
            checks['accent_text_is_readable'] = page.evaluate("getComputedStyle(document.documentElement).getPropertyValue('--accent-ink').trim().toLowerCase()") == page.evaluate("state.appearance.colors['accent-ink'].toLowerCase()")
            checks['nine_language_options'] = page.locator('#language option').count() == 9
            checks['removed_controls_absent'] = page.locator('#rapidMaster').count() == 0 and page.locator('#recoilMaster').count() == 0 and page.locator('#rapid').count() == 0 and page.locator('#radius').count() == 0 and page.locator('#accent').count() == 0 and page.locator('#recoilActive').count() == 1 and page.locator('#rapidActive').count() == 1
            initial_profile = page.evaluate('JSON.stringify(state.profile)')
            checks['page_renders_single_dark_look'] = page.evaluate("getComputedStyle(document.documentElement).backgroundColor") == 'rgb(23, 23, 23)' and page.evaluate('state.appearance.scheme') == 'dark' and page.evaluate('state.appearance.themes') is None and page.locator('#colorTheme').count() == 0
            page.locator('#phoneSettings > summary').click()
            page.screenshot(path=str(folder / 'phone-settings.png'))
            page.locator('#vertical').fill('17.8')
            response = page.request.post(info['address'] + '/api/update', headers={'Authorization': 'Bearer ' + info['token']}, data={'action': 'setGlobal', 'values': {'recoilActive': True}})
            checks['second_client_toggle_request_succeeds'] = response.ok
            page.locator('#vertical').press('Tab')
            page.wait_for_function("()=>state.globals.recoilActive===true")
            expected_profile = json.loads(initial_profile)
            expected_profile['vertical'] = 17.8
            page.wait_for_function('()=>pending===0 && state.profile.vertical===17.8')
            checks['remote_poll_after_edit_preserves_profile'] = page.evaluate('state.profile') == expected_profile
            page.evaluate("()=>mutate({action:'setGlobal',values:{recoilActive:false}})")
            page.locator('#phoneSettings > summary').click()
            page.locator('#phoneSettings > summary').click()
            page.locator('#languagePicker').click()
            page.locator('#pickerOptions button').filter(has_text='Português').click()
            page.wait_for_function("()=>document.documentElement.lang==='pt-BR' && pending===0")
            checks['portuguese_matches_desktop_id'] = page.evaluate('state.globals.language===0')
            checks['function_key_names_are_not_translated_as_errors'] = page.evaluate("t('F1')==='F1' && t('Tab')==='Tab'")
            checks['five_hotkey_rows'] = page.locator('#keys .row').count() == 5
            page.locator('#key4Picker').click()
            checks['search_hint_matches_picker_language'] = page.locator('#pickerSearch').get_attribute('placeholder') == page.locator('#pickerTitle').inner_text()
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
            page.locator('#phoneSettings').evaluate('(el)=>el.open=false')
            page.locator('#operatorPicker').click()
            group_headings = page.locator('#pickerOptions .picker-group')
            checks['operator_picker_hides_empty_favorites_group'] = group_headings.count() == 1 and group_headings.first.evaluate('el=>el.textContent') == 'All operators'
            checks['operator_picker_does_not_focus_search'] = page.evaluate("document.activeElement.id==='pickerClose'")
            checks['operator_picker_has_all_icons'] = page.locator('#pickerOptions img').count() == 78
            checks['no_phone_favorite_edit_button'] = page.locator('#favorite').count() == 0
            checks['ads_is_in_settings_only'] = page.locator('#phoneSettings #adsOnly').count() == 1 and page.locator('.output-card #adsOnly').count() == 0
            page.locator('#pickerSearch').fill('does-not-exist')
            checks['empty_search_has_feedback'] = page.locator('.picker-empty').inner_text() == 'No operators found'
            page.locator('#pickerSearch').fill('Ash')
            page.get_by_role('option', name='Ash', exact=True).click()
            page.wait_for_function('()=>state.operatorId===4 && pending===0')
            checks['operator_picker_selects_profile'] = page.locator('#operatorName').inner_text() == 'Ash'
            checks['operator_selection_returns_focus_without_keyboard'] = page.evaluate("document.activeElement.tagName!=='INPUT'")
            page.request.post(info['address'] + '/api/update', headers={'Authorization': 'Bearer ' + info['token']}, data={'action': 'setFavorite', 'operator': 4, 'enabled': True})
            page.wait_for_function("()=>state.operators.find(o=>o.id===4).favorite && pending===0")
            checks['quick_strip_appears_with_favorites'] = page.locator('#quickStrip button').count() == 1 and page.locator('#quickStrip button span').first.inner_text() == 'Ash'
            page.locator('#operatorPicker').click()
            group_headings = page.locator('#pickerOptions .picker-group')
            checks['favorite_not_duplicated_in_roster'] = page.get_by_role('option', name='Ash', exact=True).count() == 1
            checks['operator_picker_groups_favorites_and_all'] = group_headings.count() == 2 and group_headings.first.evaluate('el=>el.textContent') == 'Favorites' and group_headings.nth(1).evaluate('el=>el.textContent') == 'All operators'
            page.keyboard.press('Escape')
            page.evaluate('()=>mutate({action:"selectOperator",operator:0})')
            page.wait_for_function('()=>state.operatorId===0 && pending===0')
            page.locator('#quickStrip button').first.click()
            page.wait_for_function('()=>state.operatorId===4 && pending===0')
            checks['quick_strip_selects_operator'] = page.locator('#operatorName').inner_text() == 'Ash'
            # Exercise the actual slider and the five-millisecond buttons on both profiles.
            checks['shot_delay_is_slider_not_text'] = page.locator('#delay').get_attribute('type') == 'range' and page.locator('#delayValue').evaluate('(el)=>el.tagName') == 'OUTPUT'
            page.locator('#rapidActive').check()
            page.wait_for_function('()=>state.globals.rapidActive && pending===0')
            page.locator('#delay').focus()
            page.locator('#delay').press('Home')
            page.wait_for_function('()=>state.profile.delay===30 && pending===0')
            page.locator('#delayPlus').click()
            page.wait_for_function('()=>state.profile.delay===35 && pending===0')
            checks['delay_plus_adds_five_ms'] = page.locator('#delayValue').inner_text() == '35 ms'
            page.locator('#delayMinus').click()
            page.wait_for_function('()=>state.profile.delay===30 && pending===0')
            checks['delay_minus_subtracts_five_and_stops_at_min'] = page.locator('#delayMinus').is_disabled()
            page.locator('#delay').focus()
            page.locator('#delay').press('End')
            page.wait_for_function('()=>state.profile.delay===150 && pending===0')
            checks['delay_stops_at_max'] = page.locator('#delayPlus').is_disabled()
            page.locator('#dual').check()
            page.wait_for_function('()=>state.dual && state.weaponSlot===1 && pending===0')
            page.locator('#delay').focus()
            page.locator('#delay').press('Home')
            page.wait_for_function('()=>state.profile.delay===30 && pending===0')
            page.locator('#primary').click()
            page.wait_for_function('()=>state.weaponSlot===0 && pending===0')
            checks['delay_profiles_are_independent'] = page.evaluate('state.profile.delay===150')
            # Debounced slider edits must precede Save, even if Save is tapped immediately.
            page.evaluate("()=>{document.getElementById('delay').value=87;document.getElementById('delay').dispatchEvent(new Event('input'));return mutate({action:'save'})}")
            checks['save_flushes_slider_edit'] = page.evaluate('state.profile.delay===87 && !state.dirty')
            page.reload()
            page.wait_for_selector('#controls:not([hidden])')
            checks['slider_delay_survives_reload'] = page.locator('#delay').input_value() == '87'
            page.locator('#secondary').click()
            page.wait_for_function('()=>state.weaponSlot===1 && pending===0')
            checks['secondary_delay_survives_reload'] = page.locator('#delay').input_value() == '30'
            page.evaluate("()=>mutate({action:'setProfile',operator:4,slot:1,values:{rapid:false}})")
            checks['delay_disabled_when_profile_rapid_is_off'] = page.locator('#delay').is_disabled() and page.locator('#delayPlus').is_disabled() and page.locator('#delayMinus').is_disabled()
            page.locator('#primary').click()
            page.wait_for_function('()=>state.weaponSlot===0 && pending===0')
            page.evaluate('window.scrollTo(0, 0)')
            page.screenshot(path=str(folder / 'phone-controls.png'))
            page.locator('#phoneSettings > summary').click()
            layout_ok = True
            dialogs_fit = True
            for width in ((320, 390, 430, 768) if full else (390,)):
                page.set_viewport_size({'width': width, 'height': 844})
                for language in (range(9) if full else (1,)):
                    page.evaluate('(language)=>mutate({action:"setGlobal",values:{language}})', language)
                    layout_ok &= page.evaluate('document.documentElement.scrollWidth<=innerWidth')
                    page.locator('#languagePicker').click()
                    dialogs_fit &= page.evaluate("(()=>{const r=document.getElementById('picker').getBoundingClientRect();return r.left>=0&&r.right<=innerWidth&&r.top>=0&&r.bottom<=innerHeight})()")
                    page.keyboard.press('Escape')
                    page.locator('#operatorPicker').click()
                    dialogs_fit &= page.evaluate("(()=>{const r=document.getElementById('picker').getBoundingClientRect();return r.left>=0&&r.right<=innerWidth&&r.top>=0&&r.bottom<=innerHeight})()")
                    page.keyboard.press('Escape')
            checks['selected_languages_fit_selected_widths'] = layout_ok
            checks['language_dialog_fits_selected_widths'] = dialogs_fit
            page.set_viewport_size({'width': 390, 'height': 844})
            page.evaluate('()=>mutate({action:"setGlobal",values:{language:1}})')
            page.locator('#phoneSettings > summary').scroll_into_view_if_needed()
            page.screenshot(path=str(folder / 'phone-settings.png'))
            page.locator('#languagePicker').click()
            page.screenshot(path=str(folder / 'phone-languages.png'))
            page.keyboard.press('Escape')
            page.evaluate('()=>mutate({action:"save"})')
            checks['custom_hotkey_persists'] = json.loads((folder / 'azec_weapons.json').read_text())['secondaryKey'] == 118
            # Hold a valid poll response until after authorization is revoked.
            # A stale success must never reopen the disconnected controls.
            held_polls = []
            def hold_poll(route):
                held_polls.append((route, route.fetch()))
            page.route('**/api/state', hold_poll)
            page.evaluate('()=>{document.activeElement.blur();window.azecPendingPoll=poll()}')
            for _ in range(100):
                if held_polls:
                    break
                page.wait_for_timeout(10)
            if not held_polls:
                raise RuntimeError('Could not stage the pending session-refresh response')
            page.evaluate("()=>{token='expired-session';return mutate({action:'save'})}")
            for route, response in held_polls:
                route.fulfill(response=response)
            page.unroute('**/api/state', hold_poll)
            page.evaluate('window.azecPendingPoll')
            checks['expired_session_returns_to_pairing'] = page.locator('#pair').is_visible() and page.locator('#controls').is_hidden()
            qr_context = browser.new_context(viewport={'width': 390, 'height': 844}, is_mobile=True, has_touch=True)
            qr_page = qr_context.new_page()
            qr_page.goto(info['address'] + '/#token=' + info['token'])
            qr_page.wait_for_selector('#controls:not([hidden])')
            checks['qr_link_opens_controls_without_code'] = qr_page.locator('#pair').is_hidden()
            qr_context.close()
            checks['no_javascript_errors'] = not errors
            browser.close()
        stop(process)
        process, reopened = start(folder)
        def reopened_update(data):
            request = Request(reopened['address'] + '/api/update', data=json.dumps(data).encode(), headers={'Authorization': 'Bearer ' + reopened['token'], 'Content-Type': 'application/json'})
            with urlopen(request, timeout=5) as response:
                return json.load(response)
        primary = reopened_update({'action': 'selectOperator', 'operator': 4})
        secondary = reopened_update({'action': 'selectWeapon', 'slot': 1})
        checks['both_slider_delays_survive_app_restart'] = primary['profile']['delay'] == 87 and secondary['profile']['delay'] == 30
        report = {'passed': all(checks.values()), 'checks': checks, 'errors': errors,
                  'coverage': {'interface': 'monocode', 'languages': list(range(9)) if full else [1],
                               'widths': [320, 390, 430, 768] if full else [390]}}
        (folder / 'browser-verification.json').write_text(json.dumps(report, indent=2))
        print(json.dumps(report, indent=2))
        print('Browser artifacts: ' + str(folder))
        return report['passed']
    finally:
        stop(process)
        (folder / 'phone-info.json').unlink(missing_ok=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--full', action='store_true', help='Check all nine languages at four widths')
    args = parser.parse_args()
    raise SystemExit(0 if run(args.full) else 1)
