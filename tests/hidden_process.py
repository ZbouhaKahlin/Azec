"""Windows GUI fixtures on a noninteractive station, with process-tree cleanup.

CREATE_NO_WINDOW hides consoles only. A separate station also isolates GUI,
focus and clipboard. There is deliberately no fallback to the user's desktop.
"""
import ctypes
from ctypes import wintypes as w
import os
import subprocess
import threading
import time
import uuid
import weakref

user = ctypes.WinDLL('user32', use_last_error=True)
kernel = ctypes.WinDLL('kernel32', use_last_error=True)


def api(dll, name, args, result=w.BOOL):
    function = getattr(dll, name)
    function.argtypes, function.restype = args, result
    return function


get_station = api(user, 'GetProcessWindowStation', [], w.HANDLE)
set_station = api(user, 'SetProcessWindowStation', [w.HANDLE])
create_station = api(user, 'CreateWindowStationW', [w.LPCWSTR, w.DWORD, w.DWORD, w.LPVOID], w.HANDLE)
close_station = api(user, 'CloseWindowStation', [w.HANDLE])
create_desktop = api(user, 'CreateDesktopW', [w.LPCWSTR, w.LPCWSTR, w.LPVOID, w.DWORD, w.DWORD, w.LPVOID], w.HANDLE)
close_desktop = api(user, 'CloseDesktop', [w.HANDLE])
object_info = api(user, 'GetUserObjectInformationW', [w.HANDLE, ctypes.c_int, w.LPVOID, w.DWORD, ctypes.POINTER(w.DWORD)])
close_handle = api(kernel, 'CloseHandle', [w.HANDLE])
create_job = api(kernel, 'CreateJobObjectW', [w.LPVOID, w.LPCWSTR], w.HANDLE)
set_job = api(kernel, 'SetInformationJobObject', [w.HANDLE, ctypes.c_int, w.LPVOID, w.DWORD])
assign_job = api(kernel, 'AssignProcessToJobObject', [w.HANDLE, w.HANDLE])
terminate_job = api(kernel, 'TerminateJobObject', [w.HANDLE, w.UINT])
query_job = api(kernel, 'QueryInformationJobObject', [w.HANDLE, ctypes.c_int, w.LPVOID, w.DWORD, w.LPVOID])


class ObjectFlags(ctypes.Structure):
    _fields_ = [('inherit', w.BOOL), ('reserved', w.BOOL), ('flags', w.DWORD)]


class BasicLimits(ctypes.Structure):
    _fields_ = [('process_time', ctypes.c_int64), ('job_time', ctypes.c_int64), ('flags', w.DWORD),
                ('minimum', ctypes.c_size_t), ('maximum', ctypes.c_size_t), ('active', w.DWORD),
                ('affinity', ctypes.c_size_t), ('priority', w.DWORD), ('scheduling', w.DWORD)]


class ExtendedLimits(ctypes.Structure):
    _fields_ = [('basic', BasicLimits), ('io', ctypes.c_uint64 * 6),
                ('memory', ctypes.c_size_t * 4)]


class Accounting(ctypes.Structure):
    _fields_ = [('times', ctypes.c_int64 * 4), ('faults', w.DWORD), ('total', w.DWORD),
                ('active', w.DWORD), ('terminated', w.DWORD)]


class Startup(ctypes.Structure):
    _fields_ = [('cb', w.DWORD), ('reserved', w.LPWSTR), ('desktop', w.LPWSTR), ('title', w.LPWSTR),
                ('x', w.DWORD), ('y', w.DWORD), ('width', w.DWORD), ('height', w.DWORD),
                ('chars_x', w.DWORD), ('chars_y', w.DWORD), ('fill', w.DWORD), ('flags', w.DWORD),
                ('show', w.WORD), ('reserved_size', w.WORD), ('reserved_bytes', w.LPVOID),
                ('stdin', w.HANDLE), ('stdout', w.HANDLE), ('stderr', w.HANDLE)]


class ProcessInfo(ctypes.Structure):
    _fields_ = [('process', w.HANDLE), ('thread', w.HANDLE), ('pid', w.DWORD), ('tid', w.DWORD)]


create_process = api(kernel, 'CreateProcessW', [w.LPCWSTR,w.LPWSTR,w.LPVOID,w.LPVOID,w.BOOL,w.DWORD,
                        w.LPVOID,w.LPCWSTR,ctypes.POINTER(Startup),ctypes.POINTER(ProcessInfo)])
resume_thread = api(kernel, 'ResumeThread', [w.HANDLE], w.DWORD)
wait_process = api(kernel, 'WaitForSingleObject', [w.HANDLE,w.DWORD], w.DWORD)
exit_code = api(kernel, 'GetExitCodeProcess', [w.HANDLE,ctypes.POINTER(w.DWORD)])
terminate_process = api(kernel, 'TerminateProcess', [w.HANDLE,w.UINT])
_station_lock = threading.Lock()


def require(value):
    if not value:
        raise ctypes.WinError(ctypes.get_last_error())
    return value


class Desktop:
    def __init__(self):
        self.station = self.desktop = None
        self.desktop_name = 'Azec-tests-' + uuid.uuid4().hex
        # No other fixture launch may observe the temporary station binding.
        with _station_lock:
            original = require(get_station())
            try:
                # Unnamed stations are available without administrator rights;
                # Windows names this noninteractive station for the logon session.
                self.station = require(create_station(None, 0, 0x10000000, None))
                flags = ObjectFlags()
                require(object_info(self.station, 1, ctypes.byref(flags), ctypes.sizeof(flags), None))
                if flags.flags & 1:  # WSF_VISIBLE: only WinSta0 can be interactive.
                    raise RuntimeError('Refusing an interactive test window station')
                name = ctypes.create_unicode_buffer(512)
                require(object_info(self.station, 2, name, ctypes.sizeof(name), None))
                self.name = name.value
                require(set_station(self.station))
                # Exclude DESKTOP_SWITCHDESKTOP; this desktop must never be shown.
                self.desktop = require(create_desktop(self.desktop_name, None, None, 0, 0xff, None))
            except BaseException:
                require(set_station(original))
                self.close()
                raise
            finally:
                require(set_station(original))
        self.path = self.name + '\\' + self.desktop_name

    def close(self):
        if self.desktop:
            close_desktop(self.desktop)
            self.desktop = None
        if self.station:
            close_station(self.station)
            self.station = None


def cleanup(job, desktop, process=None):
    if job:
        terminate_job(job, 1)
        end = time.monotonic() + 5
        info = Accounting()
        while query_job(job, 1, ctypes.byref(info), ctypes.sizeof(info), None) and info.active:
            if time.monotonic() >= end:
                break
            time.sleep(.01)
        close_handle(job)
    if process:
        close_handle(process)
    desktop.close()


class HiddenProcess:
    def __init__(self, args, *, cwd, env=None, creationflags=0):
        self.args = [os.fspath(arg) for arg in args]
        self.returncode = None
        desktop = Desktop()
        job = None
        info = ProcessInfo()
        try:
            job = require(create_job(None, None))
            limits = ExtendedLimits()
            limits.basic.flags = 0x2000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
            require(set_job(job, 9, ctypes.byref(limits), ctypes.sizeof(limits)))
            startup = Startup()
            startup.cb = ctypes.sizeof(startup)
            startup.desktop = desktop.path
            command = ctypes.create_unicode_buffer(subprocess.list2cmdline(self.args))
            environment = ctypes.create_unicode_buffer('\0'.join(k+'='+str(v) for k,v in sorted((env or os.environ).items()))+'\0\0')
            # Create suspended so descendants cannot escape the cleanup job.
            flags = creationflags | 0x08000000 | 4 | 0x4000 | 0x400
            require(create_process(None,command,None,None,False,flags,environment,os.fspath(cwd),ctypes.byref(startup),ctypes.byref(info)))
            try:
                require(assign_job(job, info.process))
                if resume_thread(info.thread) == 0xffffffff:
                    raise ctypes.WinError(ctypes.get_last_error())
            except BaseException:
                terminate_process(info.process,1)
                wait_process(info.process,5000)
                raise
            self.pid, self._handle = info.pid, info.process
            self.desktop_path = desktop.path
            self._cleanup = weakref.finalize(self, cleanup, job, desktop, info.process)
        except BaseException:
            cleanup(job,desktop,info.process)
            raise
        finally:
            if info.thread:
                close_handle(info.thread)

    def poll(self):
        if self.returncode is None and wait_process(self._handle,0)==0:
            result = w.DWORD()
            require(exit_code(self._handle,ctypes.byref(result)))
            self.returncode = result.value
        return self.returncode

    def wait(self, timeout=None):
        if self.poll() is None:
            result = wait_process(self._handle,0xffffffff if timeout is None else max(0,int(timeout*1000)))
            if result==258:
                raise subprocess.TimeoutExpired(self.args,timeout)
            if result!=0:
                raise ctypes.WinError(ctypes.get_last_error())
        return self.poll()

    def close(self):
        if self._cleanup.alive:
            # cleanup kills and waits for the whole tree before releasing handles.
            self.poll()
            self._cleanup()
            if self.returncode is None:
                self.returncode = 1


def stop(process):
    if process:
        process.close()


def run(args, *, timeout, check=True, **options):
    process = HiddenProcess(args, **options)
    try:
        result = subprocess.CompletedProcess(args, process.wait(timeout=timeout))
        if check:
            result.check_returncode()
        return result
    finally:
        stop(process)
