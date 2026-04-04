from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

from common import (
    GREEN,
    GREY,
    RED,
    available_top_level_c_files,
    banner,
    colour,
    compile_source,
    expand_sections,
    headers_for_source,
    link_executable,
    parse_sections_and_defines,
    prefix,
    run_command,
    section_sources,
    select_cflags,
    source_defines_for_dir,
    unique,
    write_if_changed,
)

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
TESTS_DIR = ROOT / "tests"
BUILD_DIR = ROOT / "build"
BIN_DIR = ROOT / "_bin"
OBJ_DIR_BASE = ROOT / "_obj" / "tests"
SCRIPT_PATH = Path(__file__).resolve()
COMMON_PATH = BUILD_DIR / "common.py"

CC = os.environ.get("CC", "clang")
LDFLAGS: list[str] = []


def parse_args(argv: list[str]) -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(description="Build and run unit tests.")
    parser.add_argument(
        "tests",
        nargs="*",
        help="Test names or prefixes ending with '*' (omit to run all)",
    )
    parser.add_argument(
        "-r", "--release", action="store_true", help="Build release profile"
    )
    return parser.parse_known_args(argv[1:])


def available_tests() -> list[str]:
    return available_top_level_c_files(TESTS_DIR)


def select_tests(patterns: list[str]) -> list[str]:
    tests = available_tests()
    if not patterns:
        return tests

    selected: list[str] = []
    for pattern in patterns:
        if pattern.endswith("*"):
            prefix_text = pattern[:-1]
            matches = [test for test in tests if test.startswith(prefix_text)]
        else:
            matches = [test for test in tests if test == pattern]

        if not matches:
            raise SystemExit(colour(f"No tests matched '{pattern}'", RED))

        for match in matches:
            if match not in selected:
                selected.append(match)

    return selected


def executable_path(profile: str) -> Path:
    suffix = "-debug" if profile == "debug" else ""
    extension = ".exe" if os.name == "nt" else ""
    return BIN_DIR / f"tests{suffix}{extension}"


def selection_stamp(profile: str) -> Path:
    return OBJ_DIR_BASE / profile / ".selection"


def main(argv: list[str] | None = None) -> None:
    argv = argv or sys.argv
    args, runner_args = parse_args(argv)

    profile = "release" if args.release else "debug"
    cflags = select_cflags(profile)
    obj_dir = OBJ_DIR_BASE / profile
    include_flags = ["-Isrc", "-Ibuild"]

    tests = select_tests(args.tests)
    if not tests:
        raise SystemExit(colour("No tests found in tests/", RED))

    banner(profile, tests, "tests", CC)

    test_sources = [TESTS_DIR / f"{name}.c" for name in tests]
    root_defines: dict[Path, list[str]] = {}
    sections: list[str] = []
    for src in test_sources:
        source_sections, defines = parse_sections_and_defines(src, SRC_DIR)
        root_defines[src] = defines
        for section in source_sections:
            if section not in sections:
                sections.append(section)

    section_list = expand_sections(sections, SRC_DIR)
    dependency_sources = unique(
        src for section in section_list for src in section_sources(section, SRC_DIR)
    )
    support_source = BUILD_DIR / "test.c"
    all_sources = unique([*test_sources, support_source, *dependency_sources])

    extra_flags_by_source: dict[Path, list[str]] = {}
    header_deps_by_source: dict[Path, list[Path]] = {}
    relative_roots: dict[Path, Path] = {}
    local_build_roots: dict[Path, Path | None] = {}

    for src in all_sources:
        if src == support_source:
            extra_flags_by_source[src] = ["-DTEST"]
            header_deps_by_source[src] = [BUILD_DIR / "test.h"]
            relative_roots[src] = ROOT
            local_build_roots[src] = BUILD_DIR
            continue

        if src.is_relative_to(TESTS_DIR):
            extra_flags_by_source[src] = [
                "-DTEST",
                *[f"-D{define}" for define in root_defines[src]],
            ]
            header_deps_by_source[src] = headers_for_source(src, TESTS_DIR, SRC_DIR)
            relative_roots[src] = ROOT
            local_build_roots[src] = TESTS_DIR
            continue

        defines = source_defines_for_dir(src.parent, SRC_DIR)
        extra_flags_by_source[src] = ["-DTEST", *[f"-D{define}" for define in defines]]
        header_deps_by_source[src] = headers_for_source(src, SRC_DIR, SRC_DIR)
        relative_roots[src] = ROOT
        local_build_roots[src] = SRC_DIR

    stamp = selection_stamp(profile)
    write_if_changed(stamp, "\n".join(tests) + "\n")

    compiled: dict[Path, Path] = {}
    skipped_sources = 0
    for src in all_sources:
        obj, skipped = compile_source(
            cc=CC,
            cflags=cflags,
            include_flags=include_flags,
            obj_dir=obj_dir,
            src=src,
            relative_to=relative_roots[src],
            display_root=ROOT,
            extra_flags=extra_flags_by_source.get(src, []),
            header_deps=header_deps_by_source.get(src, []),
            extra_deps=[SCRIPT_PATH, COMMON_PATH],
            local_build_root=local_build_roots[src],
        )
        compiled[src] = obj
        if skipped:
            skipped_sources += 1

    executable = executable_path(profile)
    objects = [compiled[src] for src in all_sources]
    link_executable(
        cc=CC,
        ldflags=LDFLAGS,
        bin_dir=BIN_DIR,
        root=ROOT,
        objects=objects,
        executable=executable,
        extra_deps=[stamp],
    )

    print(f"{prefix('skip', GREY)} {skipped_sources} source file(s) up to date")
    print(f"{prefix('run', GREEN)} {executable.relative_to(ROOT)}")
    run_command([str(executable), *runner_args])


if __name__ == "__main__":
    main()
