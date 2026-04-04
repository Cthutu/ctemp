from __future__ import annotations

import argparse
import json
import os
import queue
import shutil
import subprocess
import sys
import threading
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, NamedTuple
from rich import box
from rich.console import Console
from rich.panel import Panel
from rich.table import Table

from common import (
    GREEN,
    GREY,
    RED,
    YELLOW,
    banner,
    colour,
    compile_source,
    expand_sections,
    headers_for_source,
    link_executable,
    parse_sections_and_defines,
    prefix,
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
RICH_CONSOLE = Console(stderr=False)


class TestTarget(NamedTuple):
    name: str
    sources: list[Path]


@dataclass
class FailureEvent:
    category: str
    name: str
    file: str
    line: int
    expected: str
    detail: str


@dataclass
class ModuleResult:
    name: str
    executable: Path
    total_tests: int = 0
    passed_tests: int = 0
    failed_tests: int = 0
    total_assertions: int = 0
    failed_assertions: int = 0
    categories: list[dict[str, Any]] = field(default_factory=list)
    failures: list[FailureEvent] = field(default_factory=list)
    exit_code: int = 0


def parse_args(argv: list[str]) -> argparse.Namespace:
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
    return parser.parse_args(argv[1:])


def discover_test_targets() -> list[TestTarget]:
    targets: list[TestTarget] = []
    for path in sorted(TESTS_DIR.iterdir()):
        if path.is_file() and path.suffix == ".c":
            targets.append(TestTarget(path.stem, [path]))
        elif path.is_dir():
            sources = sorted(path.rglob("*.c"))
            if sources:
                targets.append(TestTarget(path.name, sources))

    names = [target.name for target in targets]
    if len(names) != len(set(names)):
        duplicates = sorted({name for name in names if names.count(name) > 1})
        raise SystemExit(colour(f"Duplicate test target names: {', '.join(duplicates)}", RED))
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


def reader_thread(stream, channel: str, out_queue: queue.Queue[tuple[str, str]]) -> None:
    try:
        for line in iter(stream.readline, ""):
            out_queue.put((channel, line.rstrip("\n")))
    finally:
        stream.close()


def print_progress(completed: int, total: int, running: str) -> None:
    width = 24
    filled = 0 if total == 0 else int(width * completed / total)
    bar = "#" * filled + "-" * (width - filled)
    line = f"{prefix('test', YELLOW)} [{bar}] {completed}/{total} {running}"
    terminal_width = shutil.get_terminal_size(fallback=(120, 20)).columns
    if len(line) > terminal_width - 1:
        line = line[: max(0, terminal_width - 1)]
    sys.stderr.write("\r" + line.ljust(max(terminal_width - 1, len(line))))
    sys.stderr.flush()


def clear_progress() -> None:
    terminal_width = shutil.get_terminal_size(fallback=(120, 20)).columns
    sys.stderr.write("\r" + " " * max(terminal_width - 1, 1) + "\r")
    sys.stderr.flush()


def run_test_binary(executable: Path, module_name: str, runner_args: list[str]) -> ModuleResult:
    result = ModuleResult(name=module_name, executable=executable)
    cmd = [str(executable), *runner_args]
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

    event_queue: queue.Queue[tuple[str, str]] = queue.Queue()
    stdout_thread = threading.Thread(target=reader_thread, args=(process.stdout, "stdout", event_queue))
    stderr_thread = threading.Thread(target=reader_thread, args=(process.stderr, "stderr", event_queue))
    stdout_thread.start()
    stderr_thread.start()

    active = 2
    completed_tests = 0
    expected_tests = 0
    while active > 0 or not event_queue.empty():
        try:
            channel, line = event_queue.get(timeout=0.05)
        except queue.Empty:
            if process.poll() is not None and not stdout_thread.is_alive() and not stderr_thread.is_alive():
                active = 0
            continue

        if not line:
            continue
        try:
            event = json.loads(line)
        except json.JSONDecodeError:
            continue

        if event.get("event") == "suite_start":
            expected_tests = int(event.get("selected_tests", 0))
            print_progress(0, expected_tests or 1, module_name)
        elif event.get("event") == "test_end":
            completed_tests += 1
            print_progress(
                completed_tests,
                expected_tests or max(completed_tests, 1),
                f"{module_name}:{event['category']}:{event['name']}",
            )
        elif event.get("event") == "suite_end":
            result.total_tests = int(event.get("total_tests", 0))
            result.passed_tests = int(event.get("passed_tests", 0))
            result.failed_tests = int(event.get("failed_tests", 0))
            result.total_assertions = int(event.get("total_assertions", 0))
            result.failed_assertions = int(event.get("failed_assertions", 0))
            result.categories = list(event.get("categories", []))
        elif channel == "stderr" and event.get("event") == "assertion_failed":
            result.failures.append(
                FailureEvent(
                    category=str(event.get("category", "")),
                    name=str(event.get("name", "")),
                    file=str(event.get("file", "")),
                    line=int(event.get("line", 0)),
                    expected=str(event.get("expected", "")),
                    detail=str(event.get("detail", "")),
                )
            )

        if process.poll() is not None and not stdout_thread.is_alive() and not stderr_thread.is_alive():
            active = 0

    stdout_thread.join()
    stderr_thread.join()
    result.exit_code = process.wait()
    print_progress(expected_tests or completed_tests or 1, expected_tests or completed_tests or 1, module_name)
    return result


def print_module_table(results: list[ModuleResult]) -> None:
    clear_progress()
    table = Table(title="Test Modules", box=box.ROUNDED, header_style="bold cyan")
    table.add_column("Module", style="bold")
    table.add_column("Tests", justify="right")
    table.add_column("Pass", justify="right", style="green")
    table.add_column("Fail", justify="right")
    table.add_column("Assert", justify="right")
    table.add_column("A.Fail", justify="right")
    for result in results:
        fail_colour = RED if result.failed_tests else GREEN
        fail_style = "red" if result.failed_tests else "green"
        afail_style = "red" if result.failed_assertions else "green"
        table.add_row(
            result.name,
            str(result.total_tests),
            str(result.passed_tests),
            f"[{fail_style}]{result.failed_tests}[/{fail_style}]",
            str(result.total_assertions),
            f"[{afail_style}]{result.failed_assertions}[/{afail_style}]",
        )
    RICH_CONSOLE.print(table)


def print_failures(results: list[ModuleResult]) -> None:
    failures = [(result.name, failure) for result in results for failure in result.failures]
    if not failures:
        return
    table = Table(title="Failures", box=box.ROUNDED, header_style="bold red")
    table.add_column("Module", style="bold")
    table.add_column("Test")
    table.add_column("Location")
    table.add_column("Expected")
    table.add_column("Detail")
    for module_name, failure in failures:
        table.add_row(
            module_name,
            f"{failure.category}:{failure.name}",
            f"{failure.file}:{failure.line}",
            failure.expected,
            failure.detail,
        )
    RICH_CONSOLE.print(table)


def print_aggregate_summary(results: list[ModuleResult]) -> None:
    total_tests = sum(result.total_tests for result in results)
    passed_tests = sum(result.passed_tests for result in results)
    failed_tests = sum(result.failed_tests for result in results)
    total_assertions = sum(result.total_assertions for result in results)
    failed_assertions = sum(result.failed_assertions for result in results)

    table = Table(title="Summary", box=box.ROUNDED, header_style="bold magenta")
    table.add_column("Metric", style="bold")
    table.add_column("Value", justify="right")
    table.add_row("modules", str(len(results)))
    table.add_row("tests passed", f"{passed_tests}/{total_tests}")
    table.add_row("assertions", str(total_assertions))
    if failed_tests:
        table.add_row("failed tests", f"[red]{failed_tests}[/red]")
        table.add_row("failed assertions", f"[red]{failed_assertions}[/red]")
    RICH_CONSOLE.print(table)

    if failed_tests == 0:
        RICH_CONSOLE.print(
            Panel.fit(
                "[bold green]ALL TESTS PASSED[/bold green]\n[green]Ship it, nerd.[/green]",
                border_style="green",
                box=box.DOUBLE,
                padding=(1, 4),
            )
        )
    else:
        RICH_CONSOLE.print(
            Panel.fit(
                f"[bold red]{failed_tests} TEST{'S' if failed_tests != 1 else ''} FAILED[/bold red]\n"
                "[red]See the failures table above.[/red]",
                border_style="red",
                box=box.DOUBLE,
                padding=(1, 4),
            )
        )


def main(argv: list[str] | None = None) -> None:
    argv = argv or sys.argv
    args = parse_args(argv)

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
            extra_flags_by_source[src] = ["-DTEST", *[f"-D{define}" for define in root_defines[src]]]
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

    executables: list[tuple[str, Path]] = []
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
        executables.append((test_name, executable))

    print(f"{prefix('skip', GREY)} {skipped_sources} source file(s) up to date")

    runner_args: list[str] = []
    if args.test_filter:
        runner_args = ["-t", args.test_filter]

    results: list[ModuleResult] = []
    for module_name, executable in executables:
        results.append(run_test_binary(executable, module_name, runner_args))

    print_module_table(results)
    print_failures(results)
    print_aggregate_summary(results)

    if any(result.exit_code != 0 for result in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
