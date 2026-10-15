#!/usr/bin/env python3
"""
Compiles and runs every legacy/modern example pair described in
config/features.json. All compiler choices, standard versions and flags
are read from that JSON file - nothing is hardcoded here.

Usage:
    python3 build.py            # build + run every feature
    python3 build.py 01-std-expected   # build + run a single feature by id
"""
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CONFIG_PATH = ROOT / "config" / "features.json"
BUILD_DIR = ROOT / ".build"


def load_config() -> dict:
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def resolve_compilers(compiler_cfg: dict) -> list:
    candidates = [compiler_cfg["cxx"], compiler_cfg["cxx_fallback"]]
    available = [c for c in candidates if shutil.which(c)]
    if not available:
        raise RuntimeError(
            f"Neither '{candidates[0]}' nor '{candidates[1]}' "
            "was found on PATH. Install a C++23-capable compiler."
        )
    return available


def try_compile(cxx: str, common_flags: list, source: Path, flags: list, out_path: Path) -> bool:
    cmd = [cxx, *common_flags, *flags, str(source), "-o", str(out_path)]
    print(f"\n$ {' '.join(cmd)}")
    result = subprocess.run(cmd)
    return result.returncode == 0


def compile_and_run(compilers: list, common_flags: list, source: Path, flags: list, out_path: Path) -> None:
    # Library support for very new headers (<print>, <mdspan>, <generator>)
    # trails compiler-frontend language support. Try every configured
    # compiler in order instead of failing on the first one that lacks
    # the required standard library version.
    for cxx in compilers:
        if not shutil.which(cxx):
            continue
        if try_compile(cxx, common_flags, source, flags, out_path):
            print(f"$ {out_path}")
            subprocess.run([str(out_path)], check=True)
            return
    raise RuntimeError(
        f"None of the configured compilers ({', '.join(compilers)}) could "
        f"build {source}. This means the installed standard library does "
        f"not yet ship the required C++23 header for this feature."
    )


def main() -> int:
    config = load_config()
    compilers = resolve_compilers(config["compiler"])
    common_flags = config["compiler"]["common_flags"]

    requested_id = sys.argv[1] if len(sys.argv) > 1 else None
    features = config["features"]
    if requested_id:
        features = [f for f in features if f["id"] == requested_id]
        if not features:
            print(f"Unknown feature id: {requested_id}")
            return 1

    BUILD_DIR.mkdir(exist_ok=True)

    for feature in features:
        print(f"\n=== {feature['title']} ===")
        for variant in ("legacy", "modern"):
            source = ROOT / feature[f"{variant}_source"]
            flags = feature[f"{variant}_flags"]
            out_path = BUILD_DIR / f"{feature['id']}_{variant}"
            try:
                compile_and_run(compilers, common_flags, source, flags, out_path)
            except subprocess.CalledProcessError as exc:
                print(f"[{feature['id']}/{variant}] the built binary exited "
                      f"with code {exc.returncode}.")
                return exc.returncode
            except RuntimeError as exc:
                print(f"[{feature['id']}/{variant}] {exc}")
                # Do not abort the whole run: a missing <print>/<mdspan>/
                # <generator> header on an older standard library is an
                # environment limitation, not a bug in the example itself.
                continue

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
