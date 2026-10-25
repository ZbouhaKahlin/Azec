"""Focused mobile pairing, layout, and sound checks (Playwright, Pillow, qrcode)."""
import json
from pathlib import Path
import tempfile

from PIL import Image
import qrcode
from playwright.sync_api import sync_playwright
from run_checks import ROOT, start, stop


def run():
    (ROOT / '.build/verification').mkdir(parents=True, exist_ok=True)
    folder = Path(tempfile.mkdtemp(prefix='phone-companion-', dir=ROOT / '.build/verification'))
    process = None
    checks, errors, qr_images = {}, [], []
    try:
        process, info = start(folder)
        def qr(name, link):
            path = folder / name
            qrcode.make(link).save(path)
            qr_images.append(path)
            return path
        valid = qr('pairing.png', info['address'] + '/#token=' + info['token'])
        foreign = qr('foreign.png', 'https://example.invalid/#token=' + 'f' * 32)
        expired = qr('expired.png', info['address'] + '/#token=' + '0' * 32)
        blank = folder / 'blank.png'
        Image.new('RGB', (300, 300), 'white').save(blank)
        with sync_playwright() as p:
            browser = p.chromium.launch(channel='chrome', headless=True, args=['--mute-audio'])
            context = browser.new_context(viewport={'width': 390, 'height': 844}, is_mobile=True, has_touch=True)
            page = context.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            requests = []
            page.on('request', lambda request: requests.append((request.method, request.url.split('#')[0], request.post_data_buffer)))
            page.goto(info['address'])
            checks['qr_decoder_is_embedded_and_offline'] = page.evaluate("typeof jsQR==='function'&&!document.querySelector('script[src]')")
            checks['no_audio_context_on_page_load'] = page.evaluate('phoneAudio===null')
            checks['camera_photo_and_pairing_code_are_available'] = page.locator('#qrPhoto').get_attribute('capture') == 'environment' and page.locator('#code').is_visible()
            page.locator('#pair').screenshot(path=str(folder / 'pairing-options.png'))
            page.locator('#code').fill('000000')
            page.get_by_role('button', name='Connect', exact=True).click()
            page.wait_for_function('()=>!pairing')
            checks['pairing_errors_are_inline_without_popups'] = page.evaluate("!document.getElementById('pairError').hidden&&getComputedStyle(document.getElementById('pairError')).position==='static'&&!document.getElementById('notice')")
            def scan(path):
                with page.expect_file_chooser() as chooser:
                    page.locator('#scanQr').click()
                chooser.value.set_files(path)
                page.wait_for_function('()=>!pairing')
            qr_request_start = len(requests)
            scan(blank)
            checks['missing_qr_keeps_code_entry_available'] = 'No QR code found' in page.locator('#scanStatus').inner_text() and page.locator('#code').is_enabled()
            before = len(requests)
            scan(foreign)
            checks['foreign_qr_never_sends_credentials'] = len(requests) == before and page.locator('#scanStatus').inner_text() == 'Scan the QR code from this PC.'
            scan(expired)
            checks['expired_qr_stays_unpaired_with_retry'] = page.locator('#pair').is_visible() and page.locator('#scanQr').is_enabled() and page.evaluate("token===''&&sessionStorage.getItem('azec-token')===null")
            scan(valid)
            checks['real_qr_photo_pairs_without_typing_code'] = page.locator('#controls').is_visible() and page.evaluate('!!state&&state.operators.length===78')
            checks['photo_bytes_are_never_uploaded'] = all(method == 'GET' for method, _, _ in requests[qr_request_start:])
            checks['pairing_uses_only_the_local_server'] = all(url.startswith(info['address'] + '/') for _, url, _ in requests)
            checks['rapid_toggle_is_above_rapid_section_only'] = page.evaluate("$('rapidActive').closest('.rapid-card')!==null&&!document.querySelector('.output-card #rapidActive')&&$('rapidActive').closest('.row').nextElementSibling.tagName==='H2'")
            checks['delay_buttons_show_only_symbols'] = page.locator('#delayMinus').inner_text() == '−' and page.locator('#delayPlus').inner_text() == '+'
            fit = True
            for width in (320, 390):
                page.set_viewport_size({'width': width, 'height': 844})
                for delay in (99, 100, 150):
                    fit &= page.evaluate("delay=>{document.getElementById('delay').value=delay;syncDelay();const output=document.getElementById('delayValue'),range=document.createRange();range.selectNodeContents(output);return range.getClientRects().length===1&&output.scrollWidth<=output.clientWidth&&document.documentElement.scrollWidth<=innerWidth}", delay)
            checks['three_digit_delay_stays_on_one_line'] = fit
            page.locator('.rapid-card').evaluate("el=>el.scrollIntoView({block:'center'})")
            page.locator('.rapid-card').screenshot(path=str(folder / 'rapid-controls.png'))
            page.set_viewport_size({'width': 390, 'height': 844})
            page.wait_for_function("()=>phoneAudio&&phoneAudio.state==='running'")
            page.evaluate("()=>{window.phoneCueCalls=[];const original=playPhoneCue;playPhoneCue=name=>{phoneCueCalls.push(name);return original(name)}}")
            page.locator('#recoilActive').check()
            page.wait_for_function('()=>pending===0&&state.globals.recoilActive')
            checks['confirmed_toggle_schedules_one_local_cue'] = page.evaluate("phoneCueCalls.join(',')==='toggle'&&stopPhoneSound!==null")
            count = page.evaluate('phoneCueCalls.length')
            page.evaluate('()=>{render();return poll()}')
            checks['polling_and_rendering_are_silent'] = page.evaluate('phoneCueCalls.length') == count
            page.locator('#save').click()
            page.wait_for_function('()=>pending===0&&!state.dirty')
            checks['successful_save_schedules_success'] = page.evaluate("phoneCueCalls.at(-1)==='success'&&stopPhoneSound!==null")
            page.locator('#phoneSettings > summary').click()
            page.locator('#sound').uncheck()
            page.wait_for_function('()=>pending===0&&!state.globals.sound')
            checks['mute_stops_current_and_future_cues'] = page.evaluate("()=>{playPhoneCue('success');return stopPhoneSound===null}")
            page.locator('#save').click()
            page.wait_for_function('()=>pending===0&&!state.dirty')
            page.reload()
            page.wait_for_selector('#controls:not([hidden])')
            checks['shared_sound_preference_survives_save_and_reload'] = page.evaluate('!state.globals.sound&&phoneAudio===null')
            page.locator('#phoneSettings > summary').click()
            page.locator('#sound').check()
            page.wait_for_function('()=>pending===0&&state.globals.sound')
            page.wait_for_function("()=>phoneAudio&&phoneAudio.state==='running'")
            checks['enabling_sound_resumes_feedback'] = page.evaluate('stopPhoneSound!==null')
            page.evaluate("()=>{window.phoneCueCalls=[];const original=playPhoneCue;playPhoneCue=name=>{phoneCueCalls.push(name);return original(name)};return mutate({action:'setProfile',operator:state.operatorId,slot:state.weaponSlot,values:{delay:151}})}")
            checks['rejected_edit_plays_refusal_without_success'] = page.evaluate("phoneCueCalls.join(',')==='refusal'&&document.getElementById('connection').classList.contains('connected')")
            def fail_save(route):
                if route.request.post_data_json.get('action') == 'save':
                    route.abort()
                else:
                    route.continue_()
            page.route('**/api/update', fail_save)
            page.locator('#save').click()
            page.wait_for_function("()=>pending===0&&phoneErrorKind==='save'")
            checks['failed_save_shows_inline_error_and_retains_edits'] = page.evaluate("state.dirty&&!document.getElementById('saveError').hidden&&getComputedStyle(document.getElementById('saveError')).position==='static'&&!document.querySelector('#notice,.notice,.notice-dismiss')")
            page.evaluate("()=>mutate({action:'setGlobal',values:{recoilActive:!state.globals.recoilActive}})")
            checks['unrelated_success_keeps_save_failure_visible'] = page.evaluate("phoneErrorKind==='save'&&!document.getElementById('saveError').hidden")
            page.unroute('**/api/update', fail_save)
            page.locator('#save').click()
            page.wait_for_function('()=>pending===0&&!state.dirty')
            checks['successful_save_clears_inline_error_without_popup'] = page.evaluate("document.getElementById('saveError').hidden&&!document.querySelector('#notice')&&document.getElementById('save').classList.contains('saved-confirm')")
            checks['all_phone_dictionaries_cover_pairing_text'] = page.evaluate("Object.values(phoneTexts).every(map=>['Scan QR code','Or enter the pairing code','Reading QR code…','No QR code found. Try a clearer photo or enter the code.','Scan the QR code from this PC.','Could not read this image. Try another photo or enter the code.','Take a photo of the QR code shown on your PC. It stays on your phone.'].every(key=>typeof map[key]==='string'&&map[key].length>0))")
            checks['no_javascript_errors'] = not errors
            browser.close()
        report = {'passed': all(checks.values()), 'checks': checks, 'errors': errors}
        (folder / 'companion-verification.json').write_text(json.dumps(report, indent=2))
        print(json.dumps(report, indent=2))
        return report['passed']
    finally:
        stop(process)
        (folder / 'phone-info.json').unlink(missing_ok=True)
        for path in qr_images:
            path.unlink(missing_ok=True)


if __name__ == '__main__':
    raise SystemExit(0 if run() else 1)
