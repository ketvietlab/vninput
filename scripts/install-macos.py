#!/usr/bin/env python3
"""Install this build for the current account; preserve older VNInput bundles."""
import argparse
import datetime
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile


def bundle_id(bundle):
    with (bundle / "Contents/Info.plist").open("rb") as file:
        return plistlib.load(file).get("CFBundleIdentifier")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", type=Path, default=Path("build/stage"))
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--input-method-only", action="store_true", help="Update only the native input method, leaving Settings untouched.")
    args = parser.parse_args()
    if sys.platform != "darwin":
        parser.error("This installer requires macOS.")
    stage = args.stage.resolve()
    user_directory = Path.home()
    items = [
        (stage / "Input Methods/VNInput.app", user_directory / "Library/Input Methods/VNInput.app", "vn.vninput.inputmethod.VNInput"),
        (stage / "vninput-settings.app", user_directory / "Applications/VNInput Settings.app", "vn.vninput.settings"),
    ]
    if args.input_method_only:
        items = items[:1]
    tool = stage / "bin/vninput-input-source"
    if not tool.is_file():
        parser.error("Missing vninput-input-source. Run cmake --install first.")
    for source, target, expected in items:
        if not source.is_dir() or bundle_id(source) != expected:
            parser.error(f"Invalid staged app: {source}")
        if target.is_symlink() or (target.exists() and bundle_id(target) != expected):
            parser.error(f"Refusing to replace an unrelated app: {target}")
    # Do not replace executable bundles while either process is running.
    for _, target, expected in items:
        running = subprocess.run(["/usr/bin/pgrep", "-f", str(target / "Contents/MacOS")], capture_output=True)
        if running.returncode == 0:
            parser.error(f"Close {target.name} before replacing it. Switch to ABC before stopping the input method.")
    if args.dry_run:
        for source, target, _ in items:
            print(f"{source} -> {target}")
        return 0
    support = user_directory / "Library/Application Support/VNInput"
    support.mkdir(parents=True, exist_ok=True)
    backup = support / "Backups" / datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    installed = []
    try:
        with tempfile.TemporaryDirectory(prefix="install-", dir=support) as temporary:
            prepared = []
            for source, target, expected in items:
                copy = Path(temporary) / target.name
                subprocess.run(["/usr/bin/ditto", str(source), str(copy)], check=True)
                subprocess.run(["/usr/bin/codesign", "--force", "--sign", "-", str(copy)], check=True)
                subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(copy)], check=True)
                prepared.append((copy, target))
            for copy, target in prepared:
                target.parent.mkdir(parents=True, exist_ok=True)
                old = None
                if target.exists():
                    backup.mkdir(parents=True, exist_ok=True)
                    old = backup / target.name
                    os.replace(target, old)
                installed.append((target, old))
                os.replace(copy, target)
    except Exception:
        # Only remove files created by this invocation and restore its backups.
        for target, old in reversed(installed):
            if target.exists():
                shutil.rmtree(target)
            if old:
                os.replace(old, target)
        raise
    registered = subprocess.run([str(tool), "--register", str(items[0][1])], capture_output=True, text=True)
    if registered.returncode == 0:
        enabled = subprocess.run([str(tool), "--enable"], capture_output=True, text=True)
        if enabled.returncode == 0:
            print(enabled.stdout.strip())
        else:
            print("Installed; add VNInput under System Settings > Keyboard > Input Sources.")
            print(enabled.stderr.strip())
    else:
        print("Installed. macOS may need a new login before VNInput appears in Input Sources.")
        print(registered.stderr.strip())
    print("Installed:", items[0][1])
    if len(items) > 1:
        print("Settings:", items[1][1])
    print("The currently selected input source was not changed.")
    print("If System Settings was already open, quit and reopen it to refresh cached Input Sources icons.")
    if backup.exists():
        print("Previous bundles:", backup)
    return 0


if __name__ == "__main__":
    sys.exit(main())
