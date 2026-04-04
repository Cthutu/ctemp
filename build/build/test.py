from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path
from typing import NamedTuple

from common import (
    GREEN,
    GREY,
    RED,
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
)

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
TESTS_DIR = ROOT / "tests"
BUILD_DIR = ROOT / "build"
BIN_DIR = ROOT / "_bin" / "tests"
OBJ_DIR_BASE = ROOT / "_obj" / "tests"
SCRIPT_PATH = Path(__file__).resolve()
COMMON_PATH = BUILD_DIR / "common.py"

CC = os.environ.get("CC", "clang")
LDFLAGS: list[str] = []


class TestTarget(NamedTuple):
    name: str
    sources: list[Path]


def parse_args(argv: list[str]) -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(description="Build and run unit tests.")
    parser.add_argument(
        "test_modules",
        nargs="*",
        help="Optional test module names (omit to run all modules)",
    )
    parser.add_argument(
        "-r", "--release", action="store_true", help="Build release profile"
    )
    parser.add_argument(
        "-t",
        "--test",
        dest="test_filter",
        help="Run only a category or category:name within each selected module",
    )
    return parser.parse_args(argv[1:]), []


def discover_test_targets() -> list[TestTarget]:
    targets: list[TestTarget] = []

    for path in sorted(TESTS_DIR.iterdir()):
        if path.is_file() and path.suffix == ".c":
            targets.append(TestTarget(path.stem, [path]))
            continue

        if path.is_dir():
            sources = sorted(path.rglob("*.c"))
            if sources:
                targets.append(TestTarget(path.name, sources))

    names = [target.name for target in targets]
    if len(names) != len(set(names)):
        duplicates = sorted({name for name in names if names.count(name) > 1})
        duplicate_list = ", ".join(duplicates)
        raise SystemExit(colour(f"Duplicate test target names: {duplicate_list}", RED))

    return targets


def select_tests(test_modules: list[str]) -> list[str]:
    tests = [target.name for target in discover_test_targets()]
    if not test_modules:
        return tests

    selected: list[str] = []
    for test_module in test_modules:
        if test_module not in tests:
            raise SystemExit(colour(f"Unknown test module '{test_module}'", RED))
        if test_module not in selected:
            selected.append(test_module)
    return selected


def executable_path(test_name: str, profile: str) -> Path:
    suffix = "-debug" if profile == "debug" else ""
    extension = ".exe" if os.name == "nt" else ""
    return BIN_DIR / f"{test_name}{suffix}{extension}"


def main(argv: list[str] | None = None) -> None:
    argv = argv or sys.argv
    args, runner_args = parse_args(argv)

    profile = "release" if args.release else "debug"
    cflags = select_cflags(profile)
    obj_dir = OBJ_DIR_BASE / profile
    include_flags = ["-Isrc", "-Ibuild"]

    available_targets = {target.name: target for target in discover_test_targets()}
    tests = select_tests(args.test_modules)
    if not tests:
        raise SystemExit(colour("No tests found in tests/", RED))

    banner(profile, tests, "tests", CC)

    support_source = BUILD_DIR / "test.c"
    root_defines: dict[Path, list[str]] = {}
    sources_by_test: dict[str, list[Path]] = {}
    all_sources: list[Path] = [support_source]

    for test_name in tests:
        target = available_targets[test_name]
        target_sections: list[str] = []
        for src in target.sources:
            source_sections, defines = parse_sections_and_defines(src, SRC_DIR)
            root_defines[src] = defines
            for section in source_sections:
                if section not in target_sections:
                    target_sections.append(section)

        dependency_sources = unique(
            src
            for section in expand_sections(target_sections, SRC_DIR)
            for src in section_sources(section, SRC_DIR)
        )
        target_sources = unique([*target.sources, support_source, *dependency_sources])
        sources_by_test[test_name] = target_sources
        all_sources.extend(target_sources)

    all_sources = unique(all_sources)

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

    executables: list[Path] = []
    for test_name in tests:
        executable = executable_path(test_name, profile)
        objects = [compiled[src] for src in sources_by_test[test_name]]
        link_executable(
            cc=CC,
            ldflags=LDFLAGS,
            bin_dir=BIN_DIR,
            root=ROOT,
            objects=objects,
            executable=executable,
        )
        executables.append(executable)

    print(f"{prefix('skip', GREY)} {skipped_sources} source file(s) up to date")
    if args.test_filter:
        runner_args = ["-t", args.test_filter]
    for executable in executables:
        print(f"{prefix('run', GREEN)} {executable.relative_to(ROOT)}")
        run_command([str(executable), *runner_args])


if __name__ == "__main__":
    main()
