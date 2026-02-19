#!/usr/bin/env python3
# Author: watsonryan
# Purpose: Convert gfortran.txt reference output into structured CSV golden tables.

from __future__ import annotations

import argparse
import csv
from pathlib import Path

PROFILE_SECTIONS = {
    "height profile": "alt",
    "latitude profile": "glat",
    "local time profile": "stl",
    "longitude profile": "glon",
    "day of year profile": "day",
    "magnetic activity profile": "ap",
}

DWM_SECTIONS = {
    "dwm: magnetic latitude profile": "mlat",
    "dwm: magnetic local time profile": "mlt",
    "dwm: kp profile": "kp",
}


def _is_float_token(tok: str) -> bool:
    try:
        float(tok)
        return True
    except ValueError:
        return False


def parse_gfortran(lines: list[str]):
    profiles = []
    dwm = []
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i].strip().lower()
        if line in PROFILE_SECTIONS:
            scenario = line
            x_name = PROFILE_SECTIONS[line]
            i += 1
            while i < n and "quiet" not in lines[i].lower():
                i += 1
            while i < n and x_name not in lines[i].lower():
                i += 1
            i += 1
            while i < n:
                raw = lines[i].strip()
                if not raw:
                    break
                toks = raw.split()
                if len(toks) == 7 and all(_is_float_token(t) for t in toks):
                    x, qmer, qzon, dmer, dzon, tmer, tzon = map(float, toks)
                    profiles.append((scenario, x_name, x, qmer, qzon, dmer, dzon, tmer, tzon))
                i += 1
        elif line in DWM_SECTIONS:
            scenario = line
            x_name = DWM_SECTIONS[line]
            i += 1
            while i < n and "mag mer" not in lines[i].lower():
                i += 1
            i += 1
            while i < n:
                raw = lines[i].strip()
                if not raw:
                    break
                toks = raw.split()
                if len(toks) == 3 and all(_is_float_token(t) for t in toks):
                    x, mer, zon = map(float, toks)
                    dwm.append((scenario, x_name, x, mer, zon))
                i += 1
        else:
            i += 1

    return profiles, dwm


def write_profiles_csv(path: Path, rows):
    with path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["scenario", "x_name", "x", "quiet_mer", "quiet_zon", "dist_mer", "dist_zon", "total_mer", "total_zon"])
        for r in rows:
            w.writerow(r)


def write_dwm_csv(path: Path, rows):
    with path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["scenario", "x_name", "x", "mag_mer", "mag_zon"])
        for r in rows:
            w.writerow(r)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--profiles-out", required=True)
    ap.add_argument("--dwm-out", required=True)
    args = ap.parse_args()

    lines = Path(args.input).read_text().splitlines()
    profiles, dwm = parse_gfortran(lines)

    write_profiles_csv(Path(args.profiles_out), profiles)
    write_dwm_csv(Path(args.dwm_out), dwm)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
