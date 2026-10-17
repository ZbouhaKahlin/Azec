"""Exercise actual startup recovery against disposable configuration files."""
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import tempfile
import urllib.request
import urllib.error

from run_checks import start, stop

def call(info, data=None):
    request = urllib.request.Request('http://127.0.0.1:' + str(info['port']) +
        ('/api/update' if data is not None else '/api/state'),
        data=json.dumps(data).encode() if data is not None else None,
        headers={'Authorization': 'Bearer ' + info['token'], 'Content-Type': 'application/json'})
    try:
        with urllib.request.urlopen(request, timeout=5) as response:
            return response.status, json.load(response)
    except urllib.error.HTTPError as error:
        return error.code, json.load(error)

def run(parent):
    parent=Path(parent)
    folder=Path(tempfile.mkdtemp(prefix='recovery-', dir=parent))
    process=None
    original=b'broken primary configuration'
    (folder/'r6_config.bin').write_bytes(original)
    (folder/'kingboss_ui.json').write_text(json.dumps({'favorites':['Ash']}))
    (folder/'kingboss_weapons.json').write_text(json.dumps({'version':1,'operators':{
        'Ash':{'enabled':True,'initialized':True,'vertical':23.7,'horizontal':-3.4,'delay':500,
               'rapid':True,'humanize':False}}}))
    checks={}
    try:
        process,info=start(folder)
        _,state=call(info)
        ash=next(op for op in state['operators'] if op['name']=='Ash')
        checks['damaged_primary_preserved']=any(path.read_bytes()==original for path in folder.glob('r6_config.corrupt-*.bin'))
        checks['secondary_survives_primary_recovery']=ash['dual']
        checks['unversioned_favorites_load']=ash['favorite']
        checks['defaults_save_after_recovery']=call(info,{'action':'save'})[0]==200 and (folder/'r6_config.bin').stat().st_size==3734
        call(info,{'action':'setFavorite','operator':4,'enabled':True})
        checks['favorites_write_version']=json.loads((folder/'kingboss_ui.json').read_text())['version']==1
        checks['legacy_secondary_delay_preserved']=json.loads((folder/'kingboss_weapons.json').read_text())['operators']['Ash']['delay']==500
        log=(folder/'kingboss.log').read_text()
        checks['recovery_logged_without_token']='recover binary' in log and info['token'] not in log
    finally:
        stop(process)
        (folder/'phone-info.json').unlink(missing_ok=True)
    locked=Path(tempfile.mkdtemp(prefix='unreadable-',dir=parent))
    (locked/'r6_config.bin').write_bytes(original)
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,wintypes.LPVOID,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
    kernel.CreateFileW.restype=wintypes.HANDLE
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    handle=kernel.CreateFileW(str(locked/'r6_config.bin'),0x80000000,0,None,3,0x80,None)
    if handle==wintypes.HANDLE(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    process=None
    try:
        process,info=start(locked)
        checks['unreadable_file_blocks_save']=call(info,{'action':'save'})[0]==400
        checks['unreadable_file_not_quarantined']=not list(locked.glob('r6_config.corrupt-*.bin'))
    finally:
        stop(process)
        kernel.CloseHandle(handle)
        (locked/'phone-info.json').unlink(missing_ok=True)
    checks['unreadable_file_unchanged']=(locked/'r6_config.bin').read_bytes()==original
    report={'passed':all(checks.values()),'checks':checks}
    (folder/'recovery-verification.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2))
    if not report['passed']:
        raise RuntimeError('Configuration recovery verification failed')

if __name__=='__main__':
    parent=Path(__file__).resolve().parents[1]/'.build'/'verification'
    parent.mkdir(parents=True,exist_ok=True)
    run(parent)
