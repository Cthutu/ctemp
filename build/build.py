from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path
from textwrap import wrap
from typing import Iterable

# Root paths
ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
BIN_DIR = ROOT / "_bin"
OBJ_BASE = ROOT / "_obj"
OBJ_DIR: Path = OBJ_BASE
API_HEADER = ROOT / "src" / "core.h"

# Compiler command sections
CC = os.environ.get("CC", "clang")
INCLUDE_FLAGS = ["-Isrc"]
CFLAGS: list[str] = []
LDFLAGS: list[str] = []

# Colour helpers
RED = "\033[31m"
GREEN = "\033[32m"
CYAN = "\033[36m"
YELLOW = "\033[33m"
GREY = "\033[90m"
BOLD = "\033[1m"
RESET = "\033[0m"


def colour(text: str, prefix: str) -> str:
    return f"{prefix}{text}{RESET}"


def prefix(label: str, color: str) -> str:
    """Return a uniform, styled status prefix like [ cc ]."""
    return colour(f"[{label:^5}]", color + (BOLD if color != GREY else ""))


def run_command(cmd: list[str]) -> None:
    """Run a command and exit cleanly on failure."""
    result = subprocess.run(cmd)
    if result.returncode != 0:
        joined = " ".join(map(str, cmd))
        bar = colour("=" * 48, RED)
        print(bar, file=sys.stderr)
        print(f"{prefix('fail', RED)} exit {result.returncode}", file=sys.stderr)
        print(colour(joined, GREY), file=sys.stderr)
        print(bar, file=sys.stderr)
        raise SystemExit(result.returncode)


def available_projects() -> list[str]:
    """Return base names of top-level C files in src/."""
    return sorted(p.stem for p in SRC_DIR.glob("*.c"))


def headers_for_source(src: Path) -> list[Path]:
    """Return headers in the source directory and all subdirectories."""
    return sorted(src.parent.rglob("*.h"))


def obj_path(src: Path) -> Path:
    relative = src.relative_to(SRC_DIR)
    return (OBJ_DIR / relative).with_suffix(".o")


def needs_rebuild(src: Path, obj: Path) -> bool:
    if not obj.exists():
        return True

    deps = [src, API_HEADER, *headers_for_source(src)]
    obj_mtime = obj.stat().st_mtime
    return any(dep.exists() and dep.stat().st_mtime > obj_mtime for dep in deps)


def compile_source(src: Path, extra_flags: Iterable[str] = ()) -> Path:
    obj = obj_path(src)
    obj.parent.mkdir(parents=True, exist_ok=True)

    if not needs_rebuild(src, obj):
        print(f"{prefix('skip', GREY)} {src.relative_to(ROOT)}")
        return obj

    cmd = [CC, *CFLAGS, *extra_flags, *INCLUDE_FLAGS, "-c", str(src), "-o", str(obj)]
    print(f"{prefix('cc', GREEN)} {src.relative_to(ROOT)}")
    run_command(cmd)
    return obj


def link_executable(objects: list[Path], executable: Path) -> None:
    BIN_DIR.mkdir(parents=True, exist_ok=True)

    if executable.exists():
        exe_mtime = executable.stat().st_mtime
        newest_obj = max(obj.stat().st_mtime for obj in objects) if objects else exe_mtime
        if exe_mtime >= newest_obj:
            print(f"{prefix('skip', GREY)} {executable.relative_to(ROOT)} (up to date)")
            return

    cmd = [CC, "-o", str(executable), *objects, *LDFLAGS]
    print(f"{prefix('link', YELLOW)} {executable.relative_to(ROOT)}")
    run_command(cmd)


def select_cflags(profile: str) -> list[str]:
    base = ["-std=c23", "-Wall", "-Wextra", "-pipe"]
    if profile == "debug":
        return [*base, "-g", "-O0", "-DDEBUG"]
    return [*base, "-O2", "-DNDEBUG"]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build nerd projects.")
    parser.add_argument("projects", nargs="*", help="Project names (omit to build all)")
    parser.add_argument("-r", "--release", action="store_true", help="Build release profile")
    return parser.parse_args(argv[1:])


def parse_sections_and_defines(src: Path) -> tuple[list[str], list[str]]:
    """Extract section names and compile-time defines from // [ ... ] markers."""
    text = src.read_text(encoding="utf-8", errors="ignore")
    matches = re.findall(r"//\s*\[(.*?)\]", text)
    sections: list[str] = []
    defines: list[str] = []
    for raw in matches:
        for token in raw.split():
            if token == "=":
                raise SystemExit(
                    colour(
                        f"Invalid define in {src}: use *NAME=VALUE with no spaces around '='",
                        RED,
                    )
                )
            if token.startswith("*"):
                define = token[1:]
                if not define or define.endswith("="):
                    raise SystemExit(
                        colour(
                            f"Invalid define in {src}: use *NAME=VALUE with no spaces around '='",
                            RED,
                        )
                    )
                if define and define not in defines:
                    defines.append(define)
                continue
            if token not in sections:
                sections.append(token)
    return sections, defines


def module_header_for_dir(directory: Path) -> Path | None:
    """Return the single header in a module directory (non-recursive)."""
    headers = sorted(directory.glob("*.h"))
    if not headers:
        return None
    if len(headers) > 1:
        names = ", ".join(h.name for h in headers)
        raise SystemExit(colour(f"Multiple headers in module {directory}: {names}", RED))
    return headers[0]


def parse_build_file(build_file: Path) -> tuple[list[str], list[str]]:
    """Extract section names and defines from a .build file."""
    sections: list[str] = []
    defines: list[str] = []
    text = build_file.read_text(encoding="utf-8", errors="ignore")
    for line in text.splitlines():
        for token in line.split():
            if token == "=":
                raise SystemExit(
                    colour(
                        f"Invalid define in {build_file}: use *NAME=VALUE with no spaces around '='",
                        RED,
                    )
                )
            if token.startswith("*"):
                define = token[1:]
                if not define or define.endswith("="):
                    raise SystemExit(
                        colour(
                            f"Invalid define in {build_file}: use *NAME=VALUE with no spaces around '='",
                            RED,
                        )
                    )
                if define and define not in defines:
                    defines.append(define)
                continue
            if token not in sections:
                sections.append(token)
    return sections, defines


def module_config_for_dir(directory: Path) -> tuple[list[str], list[str]]:
    """Combine module deps/defines from header comments and .build file."""
    sections: list[str] = []
    defines: list[str] = []
    header = module_header_for_dir(directory)
    if header:
        h_sections, h_defines = parse_sections_and_defines(header)
        sections.extend(h_sections)
        defines.extend(h_defines)
    build_file = directory / ".build"
    if build_file.exists():
        b_sections, b_defines = parse_build_file(build_file)
        for section in b_sections:
            if section not in sections:
                sections.append(section)
        for define in b_defines:
            if define not in defines:
                defines.append(define)
    return sections, defines


def expand_sections(sections: list[str]) -> list[str]:
    ordered = list(sections)
    seen = set(sections)
    pending = list(sections)
    while pending:
        section = pending.pop(0)
        deps, _ = module_config_for_dir(SRC_DIR / section)
        for dep in deps:
            if dep not in seen:
                seen.add(dep)
                ordered.append(dep)
                pending.append(dep)
    return ordered


def section_sources(section: str) -> list[Path]:
    directory = SRC_DIR / section
    if not directory.is_dir():
        raise SystemExit(colour(f"Missing section directory: {directory}", RED))
    return sorted(directory.rglob("*.c"))


def unique(seq: Iterable[Path]) -> list[Path]:
    seen: set[Path] = set()
    ordered: list[Path] = []
    for item in seq:
        if item not in seen:
            seen.add(item)
            ordered.append(item)
    return ordered


def sources_for_project(project: str) -> tuple[list[Path], list[str]]:
    root_src = SRC_DIR / f"{project}.c"
    if not root_src.exists():
        raise SystemExit(colour(f"Unknown project '{project}' (missing {root_src})", RED))

    sections, defines = parse_sections_and_defines(root_src)
    sections = expand_sections(sections)
    sources: list[Path] = [root_src]
    for section in sections:
        sources.extend(section_sources(section))
    return unique(sources), defines


def banner(profile: str, projects: list[str]) -> None:
    bar = "=" * 48
    print(colour(bar, CYAN))
    print(colour(f" build :: {CC} :: {profile} ", BOLD + CYAN))
    name_list = ", ".join(projects)
    wrapped = wrap(name_list, width=max(1, 48 - len(" projects :: ")))
    if not wrapped:
        wrapped = ["(none)"]
    for i, line in enumerate(wrapped):
        label = " projects :: " if i == 0 else " " * len(" projects :: ")
        print(colour(f"{label}{line}", BOLD + CYAN))
    print(colour(bar, CYAN))


def executable_path(project: str, profile: str) -> Path:
    suffix = "-debug" if profile == "debug" else ""
    return BIN_DIR / f"{project}{suffix}"


def main(argv: list[str] | None = None) -> None:
    global CFLAGS, OBJ_DIR
    argv = argv or sys.argv
    args = parse_args(argv)

    profile = "release" if args.release else "debug"
    CFLAGS = select_cflags(profile)
    OBJ_DIR = OBJ_BASE / profile

    projects = args.projects or available_projects()
    if not projects:
        raise SystemExit(colour("No projects found in src/", RED))

    project_sources: dict[str, list[Path]] = {}
    project_defines: dict[str, list[str]] = {}
    for name in projects:
        sources, defines = sources_for_project(name)
        project_sources[name] = sources
        project_defines[name] = defines
    all_sources = unique(src for sources in project_sources.values() for src in sources)

    banner(profile, projects)
    if not all_sources:
        raise SystemExit(colour("No C sources found in src/", RED))

    module_define_cache: dict[Path, list[str]] = {}
    extra_flags_by_source: dict[Path, list[str]] = {}
    for src in all_sources:
        module_dir = src.parent
        if module_dir not in module_define_cache:
            _, defines = module_config_for_dir(module_dir)
            module_define_cache[module_dir] = defines
        defines = module_define_cache[module_dir]
        extra_flags_by_source[src] = [f"-D{define}" for define in defines]

    for project in projects:
        root_src = SRC_DIR / f"{project}.c"
        defines = project_defines.get(project, [])
        if defines:
            extra_flags = extra_flags_by_source.get(root_src, [])
            extra_flags_by_source[root_src] = [
                *extra_flags,
                *[f"-D{define}" for define in defines],
            ]

    compiled = {
        src: compile_source(src, extra_flags_by_source.get(src, [])) for src in all_sources
    }

    for project, sources in project_sources.items():
        objects = [compiled[src] for src in sources]
        link_executable(objects, executable_path(project, profile))

    finish_bar = colour("=" * 48, GREEN)
    print(finish_bar)
    print(colour(">> Build complete. Go be nerdy! \\o/ <<", BOLD + GREEN))
    print(finish_bar)


if __name__ == "__main__":
    main()
