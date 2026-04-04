from __future__ import annotations

import argparse
import os
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
    section_sources,
    select_cflags,
    source_defines_for_dir,
    unique,
)

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
BIN_DIR = ROOT / "_bin"
OBJ_BASE = ROOT / "_obj"
SCRIPT_PATH = Path(__file__).resolve()
COMMON_PATH = SCRIPT_PATH.parent / "common.py"

CC = os.environ.get("CC", "clang")
INCLUDE_FLAGS = ["-Isrc"]
LDFLAGS: list[str] = []


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build nerd projects.")
    parser.add_argument("projects", nargs="*", help="Project names (omit to build all)")
    parser.add_argument(
        "-r", "--release", action="store_true", help="Build release profile"
    )
    return parser.parse_args(argv[1:])


def available_projects() -> list[str]:
    return available_top_level_c_files(SRC_DIR)


def sources_for_project(project: str) -> tuple[list[Path], list[str]]:
    root_src = SRC_DIR / f"{project}.c"
    if not root_src.exists():
        raise SystemExit(
            colour(f"Unknown project '{project}' (missing {root_src})", RED)
        )

    sections, defines = parse_sections_and_defines(root_src, SRC_DIR)
    sections = expand_sections(sections, SRC_DIR)
    sources: list[Path] = [root_src]
    for section in sections:
        sources.extend(section_sources(section, SRC_DIR))
    return unique(sources), defines


def executable_path(project: str, profile: str) -> Path:
    suffix = "-debug" if profile == "debug" else ""
    extension = ".exe" if os.name == "nt" else ""
    return BIN_DIR / f"{project}{suffix}{extension}"


def main(argv: list[str] | None = None) -> None:
    argv = argv or sys.argv
    args = parse_args(argv)

    profile = "release" if args.release else "debug"
    cflags = select_cflags(profile)
    obj_dir = OBJ_BASE / profile

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
    banner(profile, projects, "projects", CC)
    if not all_sources:
        raise SystemExit(colour("No C sources found in src/", RED))

    extra_flags_by_source: dict[Path, list[str]] = {}
    header_deps_by_source: dict[Path, list[Path]] = {}
    for src in all_sources:
        defines = source_defines_for_dir(src.parent, SRC_DIR)
        extra_flags_by_source[src] = [f"-D{define}" for define in defines]
        header_deps_by_source[src] = headers_for_source(src, SRC_DIR, SRC_DIR)

    for project in projects:
        root_src = SRC_DIR / f"{project}.c"
        defines = project_defines.get(project, [])
        if defines:
            extra_flags_by_source[root_src] = [
                *extra_flags_by_source.get(root_src, []),
                *[f"-D{define}" for define in defines],
            ]

    compiled: dict[Path, Path] = {}
    skipped_sources = 0
    for src in all_sources:
        obj, skipped = compile_source(
            cc=CC,
            cflags=cflags,
            include_flags=INCLUDE_FLAGS,
            obj_dir=obj_dir,
            src=src,
            relative_to=SRC_DIR,
            display_root=SRC_DIR,
            extra_flags=extra_flags_by_source.get(src, []),
            header_deps=header_deps_by_source.get(src, []),
            extra_deps=[SCRIPT_PATH, COMMON_PATH],
            local_build_root=SRC_DIR,
        )
        compiled[src] = obj
        if skipped:
            skipped_sources += 1

    for project, sources in project_sources.items():
        objects = [compiled[src] for src in sources]
        link_executable(
            cc=CC,
            ldflags=LDFLAGS,
            bin_dir=BIN_DIR,
            root=ROOT,
            objects=objects,
            executable=executable_path(project, profile),
        )

    print(f"{prefix('skip', GREY)} {skipped_sources} source file(s) up to date")
    finish_bar = colour("=" * 48, GREEN)
    print(finish_bar)
    print(colour(">> Build complete. Go be nerdy! \\o/ <<", GREEN))
    print(finish_bar)


if __name__ == "__main__":
    main()
