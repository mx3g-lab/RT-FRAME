#!/usr/bin/env python3
"""
sync_defconfig.py — 对比 build/.config 与 defconfig+prj.conf，
只展示用户关心的差异，提示是否写回 defconfig。

监控范围：
  - defconfig 已有的 key
  - prj.conf 里的 key
  - src/ 下所有 Kconfig 文件定义的 key
  - CONFIG_RTFRAME_ 前缀的 key

用法（由 build.sh 调用）:
    python3 tools/sync_defconfig.py \\
        --target-dir targets/nxp/vmu_rt1170/cm7 \\
        --defconfig  boards/nxp/vmu_rt1170/vmu_rt1170_mimxrt1176_cm7_defconfig \\
        --dot-config build/nxp/vmu_rt1170/cm7/zephyr/.config

    python3 tools/sync_defconfig.py --target-dir targets/nxp/vmu_rt1170/cm7 ... --yes
    python3 tools/sync_defconfig.py --target-dir targets/nxp/vmu_rt1170/cm7 ... --diff-only
"""

import sys
import os
import re
import argparse

RTFRAME_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

_KCONFIG_SYM_RE = re.compile(r"^\s*(?:menu)?config\s+(\w+)", re.MULTILINE)


def scan_src_kconfig_keys(src_dir):
    keys = set()
    for root, _, files in os.walk(src_dir):
        for fname in files:
            if fname == "Kconfig" or fname.startswith("Kconfig."):
                try:
                    content = open(os.path.join(root, fname)).read()
                    for sym in _KCONFIG_SYM_RE.findall(content):
                        keys.add(f"CONFIG_{sym}")
                except OSError:
                    pass
    return keys


def parse_config(path):
    result = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" in line:
                key, _, val = line.partition("=")
                result[key.strip()] = val.strip()
    return result


def main():
    parser = argparse.ArgumentParser(description="Sync .config back to defconfig")
    parser.add_argument("--target-dir",  required=True, help="Target directory (contains prj.conf)")
    parser.add_argument("--defconfig",   required=True, help="Path to defconfig file")
    parser.add_argument("--dot-config",  required=True, help="Path to build .config")
    parser.add_argument("--yes", "-y",   action="store_true", help="Auto-accept")
    parser.add_argument("--diff-only",   action="store_true", help="Only show diff")
    args = parser.parse_args()

    target_dir      = os.path.join(RTFRAME_ROOT, args.target_dir)
    defconfig_path  = os.path.join(RTFRAME_ROOT, args.defconfig)
    dot_config_path = os.path.join(RTFRAME_ROOT, args.dot_config)
    prj_conf_path   = os.path.join(target_dir, "prj.conf")
    label           = args.target_dir

    for p in [defconfig_path, dot_config_path]:
        if not os.path.exists(p):
            print(f"[error] not found: {p}")
            sys.exit(1)

    defconfig  = parse_config(defconfig_path)
    prj_conf   = parse_config(prj_conf_path) if os.path.exists(prj_conf_path) else {}
    dot_config = parse_config(dot_config_path)

    src_keys = scan_src_kconfig_keys(os.path.join(RTFRAME_ROOT, "src"))

    watched_keys = (
        set(defconfig)
        | set(prj_conf)
        | src_keys
        | {k for k in dot_config if k.startswith("CONFIG_RTFRAME_")}
    )

    baseline = {**defconfig, **prj_conf}

    added, removed, changed = {}, {}, {}
    for key in watched_keys:
        b_val = baseline.get(key)
        d_val = dot_config.get(key)
        if b_val == d_val:
            continue
        if b_val is None:
            if d_val is not None:
                added[key] = d_val
        elif d_val is None:
            removed[key] = b_val
        else:
            changed[key] = (b_val, d_val)

    if not added and not removed and not changed:
        print(f"[{label}] defconfig is already up to date.")
        return

    if added:
        print(f"\n  + New ({len(added)}):")
        for k, v in sorted(added.items()):
            print(f"    {k}={v}")

    if removed:
        print(f"\n  - Removed ({len(removed)}):")
        for k, v in sorted(removed.items()):
            print(f"    {k}={v}")

    if changed:
        print(f"\n  ~ Changed ({len(changed)}):")
        for k, (old, new) in sorted(changed.items()):
            print(f"    {k}: {old} -> {new}")

    if args.diff_only:
        return

    print()
    answer = "y" if args.yes else input("Write back to defconfig? [y/N] ").strip().lower()
    if answer != "y":
        print("Aborted.")
        return

    with open(defconfig_path) as f:
        lines = f.readlines()

    all_diffs = {
        **{k: v for k, v in added.items()},
        **{k: None for k in removed},
        **{k: new for k, (_, new) in changed.items()},
    }

    updated = set()
    new_lines = []
    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            new_lines.append(line)
            continue
        if "=" in stripped:
            key = stripped.split("=")[0].strip()
            if key in all_diffs:
                new_val = all_diffs[key]
                new_lines.append(
                    f"# {key} is not set\n" if new_val is None else f"{key}={new_val}\n"
                )
                updated.add(key)
                continue
        new_lines.append(line)

    to_add = {k: v for k, v in added.items() if k not in updated}
    if to_add:
        new_lines.append("\n# Added by sync_defconfig\n")
        for k, v in sorted(to_add.items()):
            new_lines.append(f"{k}={v}\n")

    with open(defconfig_path, "w") as f:
        f.writelines(new_lines)

    print(f"[ok] Written to {args.defconfig}")


if __name__ == "__main__":
    main()
