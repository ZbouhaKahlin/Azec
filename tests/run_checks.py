"""Isolated behavior checks; optional UI matrix and real-time expiry checks."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from hidden_process import HiddenProcess, run as run_hidden, stop as stop_hidden

ROOT = Path(__file__).resolve().parents[1]
APP = Path(os.environ.get('AZEC_APP', ROOT / 'runtime' / 'Azec.exe'))
CHECKS = Path(os.environ.get('AZEC_CHECKS', ROOT / 'runtime' / 'Azec-checks.exe'))

def start(folder):
    process = HiddenProcess([str(APP), '--dry-run', '--phone-info=phone-info.json'], cwd=folder)
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        try:
            info = json.loads((folder / 'phone-info.json').read_text())
            if info.get('pid') == process.pid:
                return process, info
        except (OSError, ValueError):
            pass
        if process.poll() is not None:
            code=process.returncode;stop(process)
            raise RuntimeError(f'Dry-run instance exited before startup completed: {code:#x}')
        time.sleep(.05)
    stop(process)
    raise TimeoutError('Dry-run app did not write pairing info within 20 seconds')

def stop(process):
    stop_hidden(process)

def run_http(parent, slow=False):
    folder = Path(tempfile.mkdtemp(prefix='http-', dir=parent))
    process = None
    try:
        process, info = start(folder)
        subprocess.run([sys.executable, str(ROOT / 'tests' / 'phone_test.py'), str(folder)]
                       + (['--slow'] if slow else []), check=True, timeout=120 if slow else 60)
        report = json.loads((folder / 'http-verification.json').read_text())
        if not report['passed']:
            raise RuntimeError('HTTP checks reported a failure')
        duplicate = run_hidden([str(APP), '--dry-run', '--phone-info=duplicate-info.json'],
                               cwd=folder, timeout=10)
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

def run(ui=False,ui_only=False,qr_scan=False,full=False,slow=False,console=False,browser=False,jobs=2):
    started = time.monotonic()
    parent = ROOT / '.build' / 'verification'
    parent.mkdir(parents=True, exist_ok=True)
    tasks=[]
    if not ui_only:
        from recovery_test import run as run_recovery
        from preferences_restart_test import run as run_preferences
        tasks.extend([('HTTP and restart',lambda:run_http(parent,slow=slow or full)),
                      ('Recovery',lambda:run_recovery(parent)),('Preferences restart',lambda:run_preferences(parent))])
    def run_ui():
        for scale in (('1', '1.5') if full else ('1',)):
            fixture = Path(tempfile.mkdtemp(prefix='ui-goofy-' + scale + '-', dir=parent))
            env = os.environ.copy()
            env['QT_SCALE_FACTOR'] = scale
            run_hidden([str(CHECKS), '--ui-self-test']
                       + (['--all-languages'] if full else []), cwd=fixture, env=env, timeout=150)
            report = json.loads((fixture / 'ui-verification.json').read_text())
            if not report['passed']:
                raise RuntimeError('UI report failed at scale ' + scale)
            if qr_scan:
                from qr_scan import verify
                if not verify(fixture):
                    raise RuntimeError('QR decoding failed at scale ' + scale)
    if ui:tasks.append(('Desktop UI',run_ui))
    if console:tasks.append(('Console regressions',lambda:run_console(parent)))
    if browser:
        from phone_browser_test import run as run_browser
        def browser_checks():
            if not run_browser(full):raise RuntimeError('Phone browser verification failed')
        tasks.append(('Phone browser',browser_checks))
    timings={};failures=[]
    def timed(name,action):
        began=time.monotonic();action();return name,round(time.monotonic()-began,2)
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        pending={pool.submit(timed,name,action):name for name,action in tasks}
        for future in as_completed(pending):
            name=pending[future]
            try:
                _,timings[name]=future.result();print(f'PASS {name}: {timings[name]:.2f}s')
            except Exception as error:
                failures.append(name);print(f'FAIL {name}: {error}')
    report={'passed':not failures,'seconds':round(time.monotonic()-started,2),'jobs':jobs,
            'timings':timings,'failures':failures,'isolation':'noninteractive Windows station and per-process cleanup job',
            'skipped':([] if full else ['150% scale','all-language layout matrix'])+([] if (full or slow) and not ui_only else ['real-time pairing expiry'])}
    summary=parent/('suite-'+str(time.time_ns())+'.json');summary.write_text(json.dumps(report,indent=2))
    print('Verification artifacts: ' + str(parent))
    print(f'Completed in {time.monotonic() - started:.1f}s')
    if failures:raise RuntimeError('Failed groups: '+', '.join(failures))

def run_console(parent):
    env=os.environ.copy()
    qt=Path(env.get('NEON_QT_ROOT',Path(env['LOCALAPPDATA'])/'NeonManagerR6/Qt/6.8.3/msvc2022_64'))
    env['PATH']=str(qt/'bin')+os.pathsep+env['PATH']
    targets=[('sensitivity_checks','Azec-sensitivity-checks',False),('reliability_checks','Azec-reliability-checks',True),
             ('review_regressions','Azec-review-checks',True),('update_checks','Azec-update-checks',None)]
    for target,name,dry in targets:
        executable=ROOT/'.build'/('checks-'+target)/'release'/(name+'.exe')
        if not executable.exists():raise FileNotFoundError(f'{executable}: run build.cmd checks first')
        fixture=Path(tempfile.mkdtemp(prefix=target+'-',dir=parent))
        args=[str(executable)]+([str(fixture)]+(['--dry-run'] if dry else []) if dry is not None else [])
        result=subprocess.run(args,cwd=fixture,env=env,capture_output=True,text=True,timeout=90,
                              creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        (fixture/'console.log').write_text(result.stdout+result.stderr,encoding='utf-8')
        if result.returncode:raise RuntimeError(f'{name} failed; see {fixture / "console.log"}')
        print(f'PASS {name}')
    scripts=[('desktop_isolation_test.py',[sys.executable,str(ROOT/'tests/desktop_isolation_test.py')]),
             ('phone_api_test.cjs',['node','--test',str(ROOT/'tests/phone_api_test.cjs')])]
    for name,args in scripts:
        fixture=Path(tempfile.mkdtemp(prefix='console-script-',dir=parent))
        result=subprocess.run(args,capture_output=True,text=True,timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
        log=fixture/'console.log'
        log.write_text(result.stdout+result.stderr,encoding='utf-8')
        if result.returncode:raise RuntimeError(f'{name} failed; see {log}')
        print(f'PASS {name}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--ui', action='store_true')
    parser.add_argument('--ui-only', action='store_true')
    parser.add_argument('--qr-scan', action='store_true', help='Decode UI QR captures (requires opencv-python-headless)')
    parser.add_argument('--full', action='store_true', help='Include 150%% UI scaling, all UI languages, and real-time HTTP expiry')
    parser.add_argument('--slow', action='store_true', help='Include real-time HTTP expiry without expanding the UI matrix')
    parser.add_argument('--console',action='store_true',help='Run the built core, sensitivity, reliability, installer-helper and updater checks')
    parser.add_argument('--browser',action='store_true',help='Run the headless Chrome phone suite')
    parser.add_argument('--suite',action='store_true',help='Run console, behavior, desktop and browser coverage without extended matrices')
    parser.add_argument('--jobs',type=int,choices=(1,2),default=2,help='Concurrent independent groups; default 2')
    args=parser.parse_args()
    run(args.ui or args.ui_only or args.qr_scan or args.suite,args.ui_only,args.qr_scan,args.full,args.slow,
        args.console or args.suite,args.browser or args.suite,args.jobs)
