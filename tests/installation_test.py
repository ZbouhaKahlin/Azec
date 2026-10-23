"""Exercise the real shareable EXE without touching the user's installation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
import time
import urllib.request
from hidden_process import HiddenProcess, stop

ROOT = Path(__file__).resolve().parents[1]
PROFILES = ("r6_config.bin", "azec_weapons.json", "azec_ui.json")
OWNERS = {}
def wait_pid(pid, force=False):
    # Terminate only process trees owned by this fixture, never a recycled PID.
    process = OWNERS.pop(pid,None)
    if process:
        stop(process)


def api(info, values=None):
    request = urllib.request.Request(
        "http://127.0.0.1:" + str(info["port"]) + ("/api/state" if values is None else "/api/update"),
        data=None if values is None else json.dumps(values).encode(),
        headers={"Authorization": "Bearer " + info["token"], "Content-Type": "application/json"})
    with urllib.request.urlopen(request, timeout=5) as reply:
        return json.load(reply)


def hashes(folder):
    return {name: hashlib.sha256((folder / name).read_bytes()).hexdigest() for name in PROFILES}


class Installation:
    def __init__(self, parent, executable):
        self.root = Path(tempfile.mkdtemp(prefix="install-", dir=parent)) / "Friend é"
        self.downloads = self.root / "Downloads"
        self.downloads.mkdir(parents=True)
        self.exe = self.downloads / "Azec.exe"
        shutil.copy2(executable, self.exe)
        self.program = self.root / "Programs" / "Azec"
        self.data = self.root / "Data" / "Azec"
        self.env = os.environ.copy()
        self.env["AZEC_INSTALL_TEST_ROOT"] = str(self.root)
        self.env["PATH"] = os.path.join(os.environ["SystemRoot"], "System32") + os.pathsep + os.environ["SystemRoot"]
        for key in ("QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QML2_IMPORT_PATH"):
            self.env.pop(key, None)
        self.pids = []
        self.launchers = []
        self.infos = []

    def launch(self, executable=None, args=(), data=None):
        data = data or self.data
        info_file = data / "phone-info.json"
        info_file.unlink(missing_ok=True)
        process = HiddenProcess([str(executable or self.exe), *args, "--dry-run",
                                    "--phone-info=phone-info.json", "--exit-after-ms=5000"],
                                cwd=self.downloads, env=self.env)
        self.launchers.append(process)
        deadline = time.monotonic() + 45
        while time.monotonic() < deadline:
            try:
                info = json.loads(info_file.read_text())
                if info.get("dryRun"):
                    self.pids.append(info["pid"])
                    OWNERS[info['pid']] = process
                    self.infos.append(info_file)
                    process.wait(timeout=10)
                    assert process.returncode == 0, "Launcher failed"
                    api(info)
                    return info
            except (OSError, ValueError):
                pass
            time.sleep(.05)
        raise RuntimeError("Installed app did not start in its isolated data folder")

    def close(self):
        for pid in self.pids:
            wait_pid(pid, force=True)
        for process in self.launchers:
            stop(process)
        for path in self.infos:
            path.unlink(missing_ok=True)


def run(executable, smoke=False, previous=None, installed_previous=None):
    parent = ROOT / ".build" / "verification"
    parent.mkdir(parents=True, exist_ok=True)
    checks = {}
    fixtures = []
    fixture = Installation(parent, executable)
    fixtures.append(fixture)
    try:
        info = fixture.launch()
        state = api(info)
        checks["fresh_english_defaults"] = state["globals"]["language"] == 1
        checks["download_folder_contains_only_exe"] = sorted(p.name for p in fixture.downloads.iterdir()) == ["Azec.exe"]
        checks["installed_launcher_matches_download"] = fixture.program.joinpath("Azec.exe").read_bytes() == executable.read_bytes()
        checks["start_menu_shortcut_created"] = (fixture.root / "StartMenu" / "Azec.lnk").is_file()
        version = (fixture.program / "version.txt").read_text()
        runtime = fixture.program / "versions" / version / "runtime"
        checks["qt_and_https_backend_present"] = (runtime / "Qt6Core.dll").is_file() and (runtime / "tls" / "qschannelbackend.dll").is_file()
        if not smoke:
            api(info, {"action": "selectOperator", "operator": 4})
            api(info, {"action": "setProfile", "operator": 4, "slot": 0, "values": {"vertical": 17.2}})
            api(info, {"action": "setDual", "operator": 4, "enabled": True})
            api(info, {"action": "setProfile", "operator": 4, "slot": 1, "values": {"vertical": 27.7, "horizontal": -3.4}})
            api(info, {"action": "setFavorite", "operator": 4, "enabled": True})
            api(info, {"action": "setTheme", "theme": "midnight"})
            api(info, {"action": "selectOperator", "operator": 71})
            api(info, {"action": "setProfile", "operator": 71, "slot": 0, "values": {"vertical": 31.5}})
            api(info, {"action": "save"})
            saved = hashes(fixture.data)
            checks["profiles_stored_outside_program_and_downloads"] = all((fixture.data / n).is_file() and not (fixture.program / n).exists() and not (fixture.downloads / n).exists() for n in PROFILES)
            checks["binary_format_preserved"] = (fixture.data / "r6_config.bin").stat().st_size == 3734
            wait_pid(info["pid"])
            info = fixture.launch(fixture.program / "Azec.exe")
            state = api(info)
            checks["noor_selection_and_primary_reloaded"] = state["operatorId"] == 71 and abs(state["profile"]["vertical"] - 31.5) < .001
            checks["favorites_and_theme_reloaded"] = state["appearance"]["id"] == "midnight" and next(op for op in state["operators"] if op["id"] == 4)["favorite"]
            api(info, {"action": "selectOperator", "operator": 4})
            primary = api(info, {"action": "selectWeapon", "slot": 0})
            secondary = api(info, {"action": "selectWeapon", "slot": 1})
            checks["primary_secondary_isolation_after_restart"] = abs(primary["profile"]["vertical"] - 17.2) < .001 and abs(secondary["profile"]["vertical"] - 27.7) < .001
            api(info, {"action": "selectOperator", "operator": 71})
            wait_pid(info["pid"])
            # Launching another downloaded copy must never replace installed settings.
            for name in PROFILES:
                (fixture.downloads / name).write_bytes(b"unrelated portable settings")
            info = fixture.launch()
            checks["existing_profiles_never_overwritten"] = hashes(fixture.data) == saved
            wait_pid(info["pid"])
            # Repair an incomplete extraction without touching settings.
            (runtime / "Qt6Svg.dll").unlink()
            info = fixture.launch(fixture.program / "Azec.exe")
            checks["missing_dependency_repaired"] = (runtime / "Qt6Svg.dll").is_file() and hashes(fixture.data) == saved
            wait_pid(info["pid"])
            migration = Installation(parent, executable)
            fixtures.append(migration)
            for name in PROFILES:
                shutil.copy2(fixture.data / name, migration.downloads / name)
            originals = hashes(migration.downloads)
            info = migration.launch()
            checks["portable_profiles_migrated_without_changing_originals"] = hashes(migration.data) == originals == hashes(migration.downloads)
            checks["migrated_noor_settings_loaded"] = abs(api(info)["profile"]["vertical"] - 31.5) < .001
            # Exercise the exact updater handoff while its old GUI is running.
            pending = migration.data / "updates" / "Azec.new.exe"
            pending.parent.mkdir()
            shutil.copy2(executable, pending)
            info = migration.launch(pending, ("--apply-update", str(migration.program / "Azec.exe"), str(info["pid"])))
            checks["update_handoff_preserves_all_profiles"] = hashes(migration.data) == originals and abs(api(info)["profile"]["vertical"] - 31.5) < .001
            wait_pid(info["pid"])
            if previous:
                legacy = Installation(parent, previous)
                fixtures.append(legacy)
                for name in PROFILES:
                    shutil.copy2(fixture.data / name, legacy.downloads / name)
                old = legacy.launch(data=legacy.downloads)
                before_upgrade = hashes(legacy.downloads)
                pending = legacy.downloads / "Azec.new.exe"
                shutil.copy2(executable, pending)
                info = legacy.launch(pending, ("--apply-update", str(legacy.exe), str(old["pid"])))
                checks["previous_release_profiles_preserved_byte_for_byte"] = hashes(legacy.data) == before_upgrade == hashes(legacy.downloads)
                checks["previous_release_noor_profile_loaded"] = abs(api(info)["profile"]["vertical"] - 31.5) < .001
                checks["old_portable_launcher_redirects_after_update"] = legacy.exe.read_bytes() == executable.read_bytes()
                wait_pid(info["pid"])
            if installed_previous:
                modern = Installation(parent, installed_previous)
                fixtures.append(modern)
                for name in PROFILES:
                    shutil.copy2(fixture.data / name, modern.downloads / name)
                info = modern.launch()
                old_version = (modern.program / "version.txt").read_text()
                original = hashes(modern.data)
                pending = modern.data / "updates" / "Azec.new.exe"
                pending.parent.mkdir()
                shutil.copy2(executable, pending)
                info = modern.launch(pending, ("--apply-update", str(modern.program / "Azec.exe"), str(info["pid"])))
                checks["installed_upgrade_switches_version_and_keeps_previous_runtime"] = (modern.program / "version.txt").read_text() == version != old_version and (modern.program / "versions" / old_version).is_dir()
                checks["installed_upgrade_preserves_all_settings"] = hashes(modern.data) == original and abs(api(info)["profile"]["vertical"] - 31.5) < .001
                wait_pid(info["pid"])
                info = modern.launch()
                checks["older_download_cannot_downgrade_installation"] = (modern.program / "version.txt").read_text() == version and (modern.program / "Azec.exe").read_bytes() == executable.read_bytes() and hashes(modern.data) == original
                wait_pid(info["pid"])
            fixture.exe.unlink()
            info = fixture.launch(fixture.program / "Azec.exe")
            checks["installed_app_runs_after_download_is_deleted"] = abs(api(info)["profile"]["vertical"] - 31.5) < .001
            wait_pid(info["pid"])
        else:
            wait_pid(info["pid"])
        report = {"passed": all(checks.values()), "checks": checks}
        (fixture.root.parent / "installation-verification.json").write_text(json.dumps(report, indent=2))
        print(json.dumps(report, indent=2))
        print("Installation artifacts: " + str(fixture.root.parent))
        if not report["passed"]:
            raise RuntimeError("Installation checks failed")
    finally:
        for fixture in fixtures:
            fixture.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, default=ROOT / "dist" / "Azec.exe")
    parser.add_argument("--previous", type=Path)
    parser.add_argument("--installed-previous", type=Path)
    parser.add_argument("--smoke", action="store_true")
    args = parser.parse_args()
    run(args.exe.resolve(), args.smoke, args.previous.resolve() if args.previous else None,
        args.installed_previous.resolve() if args.installed_previous else None)
