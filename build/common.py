from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path
from textwrap import wrap
from typing import Iterable

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
    return colour(f"[{label:^4}]", color + (BOLD if color != GREY else ""))


def run_command(cmd: list[str]) -> None:
    sys.stdout.flush()
    sys.stderr.flush()
    result = subprocess.run(cmd)
    if result.returncode != 0:
        joined = " ".join(map(str, cmd))
        bar = colour("=" * 48, RED)
        print(bar, file=sys.stderr)
        print(f"{prefix('fail', RED)} exit {result.returncode}", file=sys.stderr)
        print(colour(joined, GREY), file=sys.stderr)
        print(bar, file=sys.stderr)
        raise SystemExit(result.returncode)


def select_cflags(profile: str) -> list[str]:
    base = ["-std=c23", "-Wall", "-Wextra", "-pipe"]
    if profile == "debug":
        return [*base, "-g", "-O0", "-DDEBUG"]
    return [*base, "-O2", "-DNDEBUG"]


def available_top_level_c_files(source_dir: Path) -> list[str]:
    return sorted(path.stem for path in source_dir.glob("*.c"))


def banner(profile: str, items: list[str], noun: str, cc: str) -> None:
    bar = "=" * 48
    print(colour(bar, CYAN))
    print(colour(f" build :: {cc} :: {profile} ", BOLD + CYAN))
    name_list = ", ".join(items)
    wrapped = wrap(name_list, width=max(1, 48 - len(f" {noun} :: ")))
    if not wrapped:
        wrapped = ["(none)"]
    for index, line in enumerate(wrapped):
        label = f" {noun} :: " if index == 0 else " " * len(f" {noun} :: ")
        print(colour(f"{label}{line}", BOLD + CYAN))
    print(colour(bar, CYAN))


def unique(seq: Iterable[Path]) -> list[Path]:
    seen: set[Path] = set()
    ordered: list[Path] = []
    for item in seq:
        if item not in seen:
            seen.add(item)
            ordered.append(item)
    return ordered


def module_root_for_path(path: Path, source_root: Path) -> Path | None:
    relative = path.relative_to(source_root)
    if not relative.parts:
        return None
    candidate = source_root / relative.parts[0]
    return candidate if candidate.is_dir() else None


def section_headers(section: str, section_root: Path) -> list[Path]:
    directory = section_root / section
    if not directory.is_dir():
        return []
    return sorted(directory.rglob("*.h"))


def _add_unique(items: list[str], value: str) -> None:
    if value not in items:
        items.append(value)


def _normalize_section(path: Path) -> str:
    return path.as_posix()


def _resolve_use_token(token: str, source: Path, section_root: Path) -> str:
    relative_candidate = source.parent / token
    if relative_candidate.is_dir() and relative_candidate.is_relative_to(section_root):
        return _normalize_section(relative_candidate.relative_to(section_root))

    root_candidate = section_root / token
    if root_candidate.is_dir():
        return _normalize_section(root_candidate.relative_to(section_root))

    return token


def _parse_command_lines(
    lines: Iterable[str],
    source: Path,
    *,
    require_prefix: bool,
    section_root: Path,
) -> tuple[list[str], list[str]]:
    sections: list[str] = []
    defines: list[str] = []
    for line_no, line in enumerate(lines, start=1):
        if require_prefix:
            match = re.match(r"\s*//>\s*(\w+)\s*:\s*(.*)$", line)
            if not match:
                continue
        else:
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            if stripped.startswith("//>"):
                stripped = stripped[3:].lstrip()
            match = re.match(r"(\w+)\s*:\s*(.*)$", stripped)
            if not match:
                raise SystemExit(
                    colour(
                        f"Invalid directive in {source}:{line_no}: expected 'command: params'",
                        RED,
                    )
                )

        command, params = match.groups()
        command = command.lower()
        tokens = params.split()

        if command == "use":
            for token in tokens:
                if "=" in token:
                    raise SystemExit(
                        colour(
                            f"Invalid module name in {source}:{line_no}: '{token}'",
                            RED,
                        )
                    )
                resolved = _resolve_use_token(token, source, section_root)
                _add_unique(sections, resolved)
        elif command == "def":
            for token in tokens:
                if token == "=" or token.startswith("=") or token.endswith("="):
                    raise SystemExit(
                        colour(
                            f"Invalid define in {source}:{line_no}: use NAME or NAME=VALUE",
                            RED,
                        )
                    )
                _add_unique(defines, token)
        else:
            border_top = colour("┌──────┬───────────────────────┐", CYAN)
            border_mid = colour("├──────┼───────────────────────┤", CYAN)
            border_bot = colour("└──────┴───────────────────────┘", CYAN)
            header = (
                colour("│ ", CYAN)
                + colour("NAME", BOLD + YELLOW)
                + colour(" │ ", CYAN)
                + colour("DESCRIPTION", BOLD + YELLOW)
                + colour("           │", CYAN)
            )
            row_use = (
                colour("│ ", CYAN)
                + colour("use ", GREEN)
                + colour(" │ ", CYAN)
                + colour("module dependencies", CYAN)
                + colour("   │", CYAN)
            )
            row_def = (
                colour("│ ", CYAN)
                + colour("def ", GREEN)
                + colour(" │ ", CYAN)
                + colour("preprocessor defines", CYAN)
                + colour("  │", CYAN)
            )
            table_lines = [
                colour("Known commands:", YELLOW),
                border_top,
                header,
                border_mid,
                row_use,
                row_def,
                border_bot,
            ]
            raise SystemExit(
                colour(f"Unknown directive in {source}:{line_no}: '{command}'", RED)
                + "\n"
                + "\n".join(table_lines)
            )
    return sections, defines


def parse_sections_and_defines(
    src: Path, section_root: Path
) -> tuple[list[str], list[str]]:
    text = src.read_text(encoding="utf-8", errors="ignore")
    return _parse_command_lines(
        text.splitlines(),
        src,
        require_prefix=True,
        section_root=section_root,
    )


def module_header_for_dir(directory: Path, section_root: Path) -> Path | None:
    if directory == section_root:
        return None

    expected = directory / f"{directory.name}.h"
    if expected.exists():
        return expected

    headers = sorted(directory.glob("*.h"))
    if not headers:
        raise SystemExit(
            colour(
                f"Missing module header in {directory}: expected {expected.name}",
                RED,
            )
        )

    names = ", ".join(header.name for header in headers)
    raise SystemExit(
        colour(
            f"Invalid module header in {directory}: expected {expected.name}; found {names}",
            RED,
        )
    )


def parse_build_file(build_file: Path, section_root: Path) -> tuple[list[str], list[str]]:
    text = build_file.read_text(encoding="utf-8", errors="ignore")
    return _parse_command_lines(
        text.splitlines(),
        build_file,
        require_prefix=False,
        section_root=section_root,
    )


def module_config_for_dir(
    directory: Path, section_root: Path
) -> tuple[list[str], list[str]]:
    sections: list[str] = []
    defines: list[str] = []
    header = module_header_for_dir(directory, section_root)
    if header:
        h_sections, h_defines = parse_sections_and_defines(header, section_root)
        sections.extend(h_sections)
        defines.extend(h_defines)
    build_file = directory / ".build"
    if build_file.exists():
        b_sections, b_defines = parse_build_file(build_file, section_root)
        for section in b_sections:
            _add_unique(sections, section)
        for define in b_defines:
            _add_unique(defines, define)
    return sections, defines


def expand_sections(sections: list[str], section_root: Path) -> list[str]:
    ordered = list(sections)
    seen = set(sections)
    pending = list(sections)
    while pending:
        section = pending.pop(0)
        deps, _ = module_config_for_dir(section_root / section, section_root)
        for dep in deps:
            if dep not in seen:
                seen.add(dep)
                ordered.append(dep)
                pending.append(dep)
    return ordered


def section_sources(section: str, section_root: Path) -> list[Path]:
    directory = section_root / section
    if not directory.is_dir():
        raise SystemExit(colour(f"Missing section directory: {directory}", RED))
    return sorted(directory.rglob("*.c"))


def dependency_sections_for_source(
    src: Path,
    source_root: Path,
    section_root: Path,
) -> list[str]:
    sections: list[str] = []
    if src.suffix == ".c" and src.exists():
        source_sections, _ = parse_sections_and_defines(src, section_root)
        for section in source_sections:
            _add_unique(sections, section)

    module_dir = src.parent
    if (
        module_dir != source_root
        and module_dir.is_relative_to(section_root)
        and module_dir != section_root
    ):
        module_sections, _ = module_config_for_dir(module_dir, section_root)
        for section in module_sections:
            _add_unique(sections, section)

    if not sections:
        return []
    return expand_sections(sections, section_root)


def headers_for_source(src: Path, source_root: Path, section_root: Path) -> list[Path]:
    headers: list[Path] = []
    module_root = module_root_for_path(src, source_root)
    if module_root is not None:
        headers.extend(sorted(module_root.rglob("*.h")))

    for section in dependency_sections_for_source(src, source_root, section_root):
        headers.extend(section_headers(section, section_root))

    return sorted(set(headers))


def source_defines_for_dir(directory: Path, section_root: Path) -> list[str]:
    if directory == section_root or not directory.is_relative_to(section_root):
        return []
    _, defines = module_config_for_dir(directory, section_root)
    return defines


def obj_path(src: Path, obj_dir: Path, relative_to: Path) -> Path:
    return (obj_dir / src.relative_to(relative_to)).with_suffix(".o")


def needs_rebuild(
    src: Path,
    obj: Path,
    *,
    header_deps: Iterable[Path] = (),
    extra_deps: Iterable[Path] = (),
    local_build_root: Path | None = None,
) -> bool:
    if not obj.exists():
        return True

    deps = [src, *header_deps, *extra_deps]
    if local_build_root is not None:
        root_build = local_build_root / ".build"
        if root_build.exists():
            deps.append(root_build)

        module_root = module_root_for_path(src, local_build_root)
        if module_root is not None:
            current = src.parent
            while current.is_relative_to(module_root):
                module_build = current / ".build"
                if module_build.exists() and module_build != root_build:
                    deps.append(module_build)
                if current == module_root:
                    break
                current = current.parent

    obj_mtime = obj.stat().st_mtime
    return any(dep.exists() and dep.stat().st_mtime > obj_mtime for dep in deps)


def compile_source(
    *,
    cc: str,
    cflags: Iterable[str],
    include_flags: Iterable[str],
    obj_dir: Path,
    src: Path,
    relative_to: Path,
    display_root: Path,
    extra_flags: Iterable[str] = (),
    header_deps: Iterable[Path] = (),
    extra_deps: Iterable[Path] = (),
    local_build_root: Path | None = None,
) -> tuple[Path, bool]:
    obj = obj_path(src, obj_dir, relative_to)
    obj.parent.mkdir(parents=True, exist_ok=True)

    if not needs_rebuild(
        src,
        obj,
        header_deps=header_deps,
        extra_deps=extra_deps,
        local_build_root=local_build_root,
    ):
        return obj, True

    cmd = [cc, *cflags, *extra_flags, *include_flags, "-c", str(src), "-o", str(obj)]
    print(f"{prefix('cc', GREEN)} {src.relative_to(display_root)}")
    run_command(cmd)
    return obj, False


def link_executable(
    *,
    cc: str,
    ldflags: Iterable[str],
    bin_dir: Path,
    root: Path,
    objects: list[Path],
    executable: Path,
    extra_deps: Iterable[Path] = (),
) -> None:
    bin_dir.mkdir(parents=True, exist_ok=True)

    if executable.exists():
        exe_mtime = executable.stat().st_mtime
        dep_times = [obj.stat().st_mtime for obj in objects]
        dep_times.extend(dep.stat().st_mtime for dep in extra_deps if dep.exists())
        newest_dep = max(dep_times, default=exe_mtime)
        if exe_mtime >= newest_dep:
            print(f"{prefix('skip', GREY)} {executable.relative_to(root)} (up to date)")
            return

    cmd = [cc, "-o", str(executable), *objects, *ldflags]
    print(f"{prefix('link', YELLOW)} {executable.relative_to(root)}")
    run_command(cmd)


def write_if_changed(path: Path, content: str) -> None:
    current = None
    if path.exists():
        current = path.read_text(encoding="utf-8")
    if current == content:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
