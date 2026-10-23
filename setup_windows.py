#!/usr/bin/env python3
"""Download portable Windows tools used by FLACORE."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path, PurePosixPath


USER_AGENT = "FLACORE-Windows-Setup/0.4.2"
API_ROOT = "https://api.github.com/repos"


def request(url: str):
    req = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": USER_AGENT,
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    return urllib.request.urlopen(req, timeout=60)


def get_release(repository: str, tag: str | None = None) -> dict:
    endpoint = f"{API_ROOT}/{repository}/releases/"
    endpoint += f"tags/{tag}" if tag else "latest"
    with request(endpoint) as response:
        return json.load(response)


def get_asset(release: dict, name: str) -> dict:
    for asset in release.get("assets", []):
        if asset.get("name") == name:
            return asset
    raise RuntimeError(f"Release asset not found: {name}")


def checksum_for(release: dict, asset: dict, work_dir: Path) -> tuple[str, str]:
    digest = asset.get("digest", "")
    if ":" in digest:
        algorithm, value = digest.split(":", 1)
        if algorithm.lower() in ("sha256", "sha512"):
            return algorithm.lower(), value.lower()

    names = {
        "SHA2-512SUMS",
        "checksums.sha256",
        f"{asset['name']}.sha256sum",
    }
    checksum_asset = next(
        (item for item in release.get("assets", []) if item.get("name") in names),
        None,
    )
    if checksum_asset is None:
        raise RuntimeError(f"No published checksum found for {asset['name']}")

    checksum_path = work_dir / checksum_asset["name"]
    download(checksum_asset["browser_download_url"], checksum_path, quiet=True)
    content = checksum_path.read_text(encoding="utf-8", errors="replace")
    for line in content.splitlines():
        match = re.search(r"\b([0-9a-fA-F]{128}|[0-9a-fA-F]{64})\b(?:\s+\*?(.+))?", line)
        if not match:
            continue
        filename = (match.group(2) or "").strip()
        if not filename or PurePosixPath(filename.replace("\\", "/")).name == asset["name"]:
            value = match.group(1).lower()
            return ("sha256" if len(value) == 64 else "sha512", value)
    raise RuntimeError(f"Could not read checksum for {asset['name']}")


def download(url: str, destination: Path, quiet: bool = False) -> None:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix(destination.suffix + ".part")
    try:
        with urllib.request.urlopen(req, timeout=60) as response, temporary.open("wb") as out:
            total = int(response.headers.get("Content-Length", "0"))
            received = 0
            while True:
                chunk = response.read(1024 * 1024)
                if not chunk:
                    break
                out.write(chunk)
                received += len(chunk)
                if not quiet and total:
                    print(
                        f"\r  {destination.name}: {received * 100 // total:3d}%",
                        end="",
                        flush=True,
                    )
        if not quiet:
            print(f"\r  Downloaded {destination.name} ({received // (1024 * 1024)} MB)")
        os.replace(temporary, destination)
    except Exception:
        temporary.unlink(missing_ok=True)
        raise


def fetch_verified(release: dict, asset: dict, work_dir: Path) -> Path:
    archive = work_dir / asset["name"]
    algorithm, expected = checksum_for(release, asset, work_dir)
    download(asset["browser_download_url"], archive)

    hasher = hashlib.new(algorithm)
    with archive.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            hasher.update(chunk)
    if hasher.hexdigest().lower() != expected:
        archive.unlink(missing_ok=True)
        raise RuntimeError(f"Checksum verification failed for {asset['name']}")
    print(f"  Checksum verified ({algorithm})")
    return archive


def extract_executables(archive: Path, targets: dict[str, Path]) -> None:
    with zipfile.ZipFile(archive) as package:
        entries = {}
        for info in package.infolist():
            if info.is_dir():
                continue
            member = PurePosixPath(info.filename.replace("\\", "/"))
            basename = member.name.lower()
            if basename in targets and (basename == "deno.exe" or "bin" in member.parts):
                entries.setdefault(basename, info)

        missing = set(targets) - set(entries)
        if missing:
            raise RuntimeError(f"Archive is missing: {', '.join(sorted(missing))}")

        for basename, destination in targets.items():
            destination.parent.mkdir(parents=True, exist_ok=True)
            temporary = destination.with_suffix(destination.suffix + ".part")
            with package.open(entries[basename]) as source, temporary.open("wb") as out:
                shutil.copyfileobj(source, out)
            os.replace(temporary, destination)
            print(f"  Installed {destination}")


def install_binary(release: dict, asset_name: str, destination: Path,
                   work_dir: Path, force: bool) -> None:
    if destination.exists() and not force:
        print(f"Already present: {destination}")
        return
    asset = get_asset(release, asset_name)
    archive = fetch_verified(release, asset, work_dir)
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix(destination.suffix + ".part")
    shutil.copyfile(archive, temporary)
    os.replace(temporary, destination)
    print(f"  Installed {destination}")


def install_zip(release: dict, asset_name: str, targets: dict[str, Path],
                work_dir: Path, force: bool) -> None:
    if all(path.exists() for path in targets.values()) and not force:
        print(f"Already present: {', '.join(str(path) for path in targets.values())}")
        return
    asset = get_asset(release, asset_name)
    archive = fetch_verified(release, asset, work_dir)
    extract_executables(archive, targets)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Install portable yt-dlp and Deno beside FLACORE."
    )
    parser.add_argument(
        "--install-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "tools",
        help="directory for downloaded tools (default: ./tools)",
    )
    parser.add_argument(
        "--ffmpeg",
        action="store_true",
        help="also download the large FFmpeg package for audio conversion",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="replace tools that are already present",
    )
    args = parser.parse_args()

    if sys.platform != "win32":
        print("This setup script is for Windows.", file=sys.stderr)
        return 2

    tool_dir = args.install_dir.resolve()
    tool_dir.mkdir(parents=True, exist_ok=True)
    try:
        with tempfile.TemporaryDirectory(prefix="flacore-setup-") as temporary:
            work_dir = Path(temporary)
            print("Getting the latest yt-dlp...")
            install_binary(
                get_release("yt-dlp/yt-dlp"),
                "yt-dlp.exe",
                tool_dir / "yt-dlp.exe",
                work_dir,
                args.force,
            )

            print("Getting the latest Deno runtime...")
            deno_release = get_release("denoland/deno")
            install_zip(
                deno_release,
                "deno-x86_64-pc-windows-msvc.zip",
                {"deno.exe": tool_dir / "deno.exe"},
                work_dir,
                args.force,
            )

            if args.ffmpeg:
                print("Getting FFmpeg (large download)...")
                ffmpeg_release = get_release("BtbN/FFmpeg-Builds", "latest")
                install_zip(
                    ffmpeg_release,
                    "ffmpeg-master-latest-win64-gpl.zip",
                    {
                        "ffmpeg.exe": tool_dir / "ffmpeg" / "bin" / "ffmpeg.exe",
                        "ffprobe.exe": tool_dir / "ffmpeg" / "bin" / "ffprobe.exe",
                    },
                    work_dir,
                    args.force,
                )
    except Exception as error:
        print(f"\nSetup failed: {error}", file=sys.stderr)
        return 1

    print("\nSetup complete. FLACORE can find the tools in this folder automatically.")
    if not args.ffmpeg:
        print("Run this script with --ffmpeg if you also need audio conversion.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
