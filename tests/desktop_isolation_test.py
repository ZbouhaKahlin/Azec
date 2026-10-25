"""Prove GUI/clipboard isolation and descendant cleanup before running Qt tests."""
import ctypes
from ctypes import wintypes as w
import json
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

import hidden_process as hidden

CHILD = r'''
import ctypes,json,pathlib,subprocess,sys,time
from ctypes import wintypes as w
u=ctypes.WinDLL('user32',use_last_error=True)
u.GetProcessWindowStation.restype=w.HANDLE
u.GetUserObjectInformationW.argtypes=[w.HANDLE,ctypes.c_int,w.LPVOID,w.DWORD,w.LPVOID]
class Flags(ctypes.Structure):
 _fields_=[('inherit',w.BOOL),('reserved',w.BOOL),('flags',w.DWORD)]
flags=Flags()
assert u.GetUserObjectInformationW(u.GetProcessWindowStation(),1,ctypes.byref(flags),ctypes.sizeof(flags),None)
assert not flags.flags&1, 'Refusing to create a window on an interactive station'
u.CreateWindowExW.argtypes=[w.DWORD,w.LPCWSTR,w.LPCWSTR,w.DWORD,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,w.HWND,w.HANDLE,w.HANDLE,w.LPVOID]
u.CreateWindowExW.restype=w.HWND
u.ShowWindow.argtypes=[w.HWND,ctypes.c_int]
u.IsWindowVisible.argtypes=[w.HWND]
window=u.CreateWindowExW(0,'STATIC','Azec isolated fixture',0xcf0000,0,0,320,200,None,None,None,None)
assert window
u.ShowWindow(window,5)
descendant=subprocess.Popen([sys.executable,'-c','import time; time.sleep(60)'],creationflags=subprocess.CREATE_NO_WINDOW)
report=pathlib.Path(sys.argv[1])
pending=report.with_suffix('.tmp')
pending.write_text(json.dumps({'noninteractive':True,'window_created':bool(window),'descendant':descendant.pid}))
pending.replace(report)
time.sleep(60)
'''


class DesktopIsolation(unittest.TestCase):
    def test_fixture_window_is_noninteractive_and_tree_is_cleaned(self):
        parent = Path(__file__).resolve().parents[1]/'.build'/'verification'
        parent.mkdir(parents=True, exist_ok=True)
        folder = Path(tempfile.mkdtemp(prefix='desktop-isolation-', dir=parent))
        report = folder/'probe.json'
        original = hidden.get_station()
        process = hidden.HiddenProcess([sys.executable, '-c', CHILD, str(report)], cwd=folder)
        descendant = None
        try:
            end = time.monotonic()+10
            while not report.exists() and process.poll() is None and time.monotonic()<end:
                time.sleep(.02)
            self.assertTrue(report.exists(), 'Isolated child did not report success')
            result = json.loads(report.read_text())
            self.assertTrue(result['noninteractive'] and result['window_created'])
            self.assertEqual(hidden.get_station(), original)
            descendant = hidden.api(hidden.kernel, 'OpenProcess', [w.DWORD,w.BOOL,w.DWORD],w.HANDLE)(0x100000,False,result['descendant'])
            self.assertTrue(descendant)
        finally:
            hidden.stop(process)
        try:
            wait = hidden.api(hidden.kernel,'WaitForSingleObject',[w.HANDLE,w.DWORD],w.DWORD)
            self.assertEqual(wait(descendant,5000),0,'Fixture descendant survived job cleanup')
        finally:
            if descendant:
                hidden.close_handle(descendant)

    def test_isolation_failure_prevents_launch(self):
        with patch.object(hidden, 'create_station', return_value=None), patch.object(hidden, 'create_process') as resumed:
            with self.assertRaises(OSError):
                hidden.HiddenProcess([sys.executable, '-c', 'raise SystemExit(99)'],cwd=Path.cwd())
            resumed.assert_not_called()


if __name__ == '__main__':
    unittest.main()
