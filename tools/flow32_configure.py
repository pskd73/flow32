#!/usr/bin/env python3
"""Resolve Flow32 modules and export a physically minimal Arduino library."""

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REGISTRY = ROOT / "modules.json"
DEFAULT_CONFIG = ROOT / "flow32.config.json"
MODULE_NAME = re.compile(r"^[a-z][a-z0-9-]*$")
DEFINE_NAME = re.compile(r"^FLOW32_[A-Z0-9_]+$")
GENERATED_MARKER = ".flow32-generated"


class ConfigError(ValueError):
    pass


def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except OSError as error:
        raise ConfigError(f"cannot read {path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ConfigError(f"invalid JSON in {path}: {error}") from error


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_modules(registry, requested):
    modules = registry["modules"]
    resolved = []
    visiting = set()
    visited = set()

    def visit(name):
        if name not in modules:
            raise ConfigError(f"unknown module {name!r}")
        if name in visiting:
            raise ConfigError(f"module dependency cycle at {name!r}")
        if name in visited:
            return
        visiting.add(name)
        for dependency in modules[name]["requires"]:
            visit(dependency)
        visiting.remove(name)
        visited.add(name)
        resolved.append(name)

    for module in requested:
        visit(module)
    return resolved


def validate_registry(registry, root=ROOT):
    if not isinstance(registry, dict) or registry.get("schema") != 1:
        raise ConfigError("module registry schema must be 1")
    modules = registry.get("modules")
    profiles = registry.get("profiles")
    common = registry.get("common_files", [])
    if not isinstance(modules, dict) or not modules:
        raise ConfigError("module registry needs a non-empty modules object")
    if not isinstance(profiles, dict) or not profiles:
        raise ConfigError("module registry needs a non-empty profiles object")
    if not isinstance(common, list):
        raise ConfigError("common_files must be an array")

    owners = {}
    for name, module in modules.items():
        if not MODULE_NAME.fullmatch(name) or not isinstance(module, dict):
            raise ConfigError(f"invalid module name or record {name!r}")
        requires = module.get("requires")
        files = module.get("files")
        headers = module.get("public_headers")
        if not isinstance(requires, list) or not all(isinstance(item, str) for item in requires):
            raise ConfigError(f"module {name!r} requires must be an array of names")
        if not isinstance(files, list) or not files:
            raise ConfigError(f"module {name!r} files must be a non-empty array")
        if not isinstance(headers, list):
            raise ConfigError(f"module {name!r} public_headers must be an array")
        for relative in files:
            if not isinstance(relative, str) or not (root / relative).is_file():
                raise ConfigError(f"module {name!r} references missing file {relative!r}")
            if relative in owners:
                raise ConfigError(
                    f"file {relative!r} belongs to both {owners[relative]!r} and {name!r}"
                )
            owners[relative] = name
        for header in headers:
            if not isinstance(header, str) or not (root / "src" / header).is_file():
                raise ConfigError(f"module {name!r} has missing public header {header!r}")

    for relative in common:
        if not isinstance(relative, str) or not (root / relative).is_file():
            raise ConfigError(f"common_files references missing file {relative!r}")

    for name, module in modules.items():
        closure = resolve_modules(registry, [name])
        available = {
            relative
            for dependency in closure
            for relative in modules[dependency]["files"]
        }
        for header in module["public_headers"]:
            if f"src/{header}" not in available:
                raise ConfigError(
                    f"public header {header!r} is outside module {name!r}'s dependency closure"
                )
    for name, profile in profiles.items():
        if not MODULE_NAME.fullmatch(name) or not isinstance(profile, dict):
            raise ConfigError(f"invalid profile {name!r}")
        selected = profile.get("modules")
        if not isinstance(selected, list) or not selected:
            raise ConfigError(f"profile {name!r} needs a non-empty modules array")
        resolve_modules(registry, selected)

    registered_sources = {
        relative
        for relative in owners
        if relative.startswith("src/") and Path(relative).suffix in (".h", ".cpp")
    }
    actual_sources = {
        path.relative_to(root).as_posix()
        for path in (root / "src").rglob("*")
        if path.is_file() and path.suffix in (".h", ".cpp")
    }
    missing = sorted(actual_sources - registered_sources)
    stale = sorted(registered_sources - actual_sources)
    if missing:
        raise ConfigError("source files missing from modules.json: " + ", ".join(missing))
    if stale:
        raise ConfigError("non-source module entries under src/: " + ", ".join(stale))
    return True


def resolve_config(registry, config):
    if not isinstance(config, dict) or config.get("schema") != 1:
        raise ConfigError("configuration schema must be 1")
    profile_name = config.get("profile")
    requested = []
    if profile_name is not None:
        if profile_name not in registry["profiles"]:
            raise ConfigError(f"unknown profile {profile_name!r}")
        requested.extend(registry["profiles"][profile_name]["modules"])
    extra = config.get("modules", [])
    if not isinstance(extra, list) or not all(isinstance(item, str) for item in extra):
        raise ConfigError("modules must be an array of module names")
    requested.extend(extra)
    if not requested:
        raise ConfigError("select a profile or at least one module")

    defines = config.get("defines", {})
    if not isinstance(defines, dict):
        raise ConfigError("defines must be an object")
    normalized_defines = {}
    for name, value in defines.items():
        if not DEFINE_NAME.fullmatch(name):
            raise ConfigError(f"invalid define name {name!r}")
        if isinstance(value, bool):
            value = 1 if value else 0
        if not isinstance(value, int) or value < 0 or value > 0xFFFFFFFF:
            raise ConfigError(f"define {name!r} must be an integer in 0..4294967295")
        normalized_defines[name] = value
    return profile_name, requested, resolve_modules(registry, requested), normalized_defines


def module_macro(name):
    return "FLOW32_MODULE_" + name.upper().replace("-", "_")


def selected_header(registry, resolved):
    headers = []
    for name in resolved:
        for header in registry["modules"][name]["public_headers"]:
            if header not in headers:
                headers.append(header)
    lines = [
        "#pragma once",
        "",
        "/** Generated by tools/flow32_configure.py; edit flow32.config.json. */",
        '#include "Flow32BuildConfig.h"',
    ]
    lines.extend(f'#include "{header}"' for header in headers)
    lines.append("")
    return "\n".join(lines)


def build_config_header(registry, resolved, defines):
    enabled = set(resolved)
    lines = [
        "#pragma once",
        "",
        "/** Generated by tools/flow32_configure.py; do not edit directly. */",
    ]
    for name in registry["modules"]:
        lines.append(f"#define {module_macro(name)} {1 if name in enabled else 0}")
    if defines:
        lines.append("")
        for name, value in sorted(defines.items()):
            lines.append(f"#ifndef {name}")
            lines.append(f"#define {name} {value}")
            lines.append("#endif")
    lines.append("")
    return "\n".join(lines)


def copy_file(root, stage, relative):
    destination = stage / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / relative, destination)


def rewrite_metadata(stage):
    properties = stage / "library.properties"
    lines = properties.read_text(encoding="utf-8").splitlines()
    lines = ["includes=Flow32Selected.h" if line.startswith("includes=") else line for line in lines]
    properties.write_text("\n".join(lines) + "\n", encoding="utf-8")

    metadata_path = stage / "library.json"
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    metadata["headers"] = ["Flow32Selected.h"]
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")


def safe_destination(output, root):
    output = output.expanduser().resolve()
    root = root.resolve()
    if output == Path(output.anchor) or output == Path.home().resolve():
        raise ConfigError(f"refusing unsafe output directory {output}")
    if output == root or root in output.parents:
        raise ConfigError("output must be outside the Flow32 source directory")
    return output


def export_library(registry, config, output, force=False, root=ROOT):
    profile, requested, resolved, defines = resolve_config(registry, config)
    output = safe_destination(Path(output), root)
    if output.exists() and not force:
        raise ConfigError(f"output already exists: {output}; use --force to refresh it")
    if output.exists() and not (output / GENERATED_MARKER).is_file():
        raise ConfigError(f"refusing to replace non-generated directory: {output}")

    output.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=f".{output.name}.stage-", dir=str(output.parent)))
    try:
        selected_files = []
        for relative in registry.get("common_files", []):
            copy_file(root, stage, relative)
            selected_files.append(relative)
        for name in resolved:
            for relative in registry["modules"][name]["files"]:
                copy_file(root, stage, relative)
                selected_files.append(relative)

        (stage / "src").mkdir(parents=True, exist_ok=True)
        (stage / "src/Flow32Selected.h").write_text(
            selected_header(registry, resolved), encoding="utf-8"
        )
        (stage / "src/Flow32BuildConfig.h").write_text(
            build_config_header(registry, resolved, defines), encoding="utf-8"
        )
        rewrite_metadata(stage)

        (stage / "flow32.config.json").write_text(
            json.dumps(config, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        upstream_hashes = {
            relative: sha256(root / relative) for relative in sorted(set(selected_files))
        }
        output_hashes = {
            path.relative_to(stage).as_posix(): sha256(path)
            for path in sorted(stage.rglob("*"))
            if path.is_file()
        }
        lock = {
            "schema": 1,
            "profile": profile,
            "requested_modules": requested,
            "resolved_modules": resolved,
            "defines": defines,
            "registry_sha256": sha256(root / "modules.json"),
            "upstream_sha256": upstream_hashes,
            "output_sha256": output_hashes,
        }
        (stage / "flow32.lock.json").write_text(
            json.dumps(lock, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        (stage / GENERATED_MARKER).write_text(
            json.dumps({"schema": 1, "generator": "flow32_configure.py"}) + "\n",
            encoding="utf-8",
        )

        backup = None
        if output.exists():
            backup = output.with_name(output.name + ".previous")
            if backup.exists():
                raise ConfigError(f"remove stale generated backup first: {backup}")
            output.rename(backup)
        try:
            stage.rename(output)
        except Exception:
            if backup and backup.exists() and not output.exists():
                backup.rename(output)
            raise
        if backup:
            shutil.rmtree(backup)
        return {
            "output": str(output),
            "profile": profile,
            "requested": requested,
            "resolved": resolved,
            "files": len(set(selected_files)) + 6,
        }
    except Exception:
        if stage.exists():
            shutil.rmtree(stage)
        raise


def print_catalog(registry):
    print("Profiles:")
    for name, profile in registry["profiles"].items():
        print(f"  {name:24} {profile.get('description', '')}")
    print("\nModules:")
    for name, module in registry["modules"].items():
        dependencies = ", ".join(module["requires"]) or "none"
        print(f"  {name:24} requires: {dependencies}")
        print(f"  {'':24} {module.get('description', '')}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--registry", type=Path, default=DEFAULT_REGISTRY)
    parser.add_argument("--output", type=Path, help="curated Arduino library directory")
    parser.add_argument("--list", action="store_true", help="list profiles and modules")
    parser.add_argument("--check", action="store_true", help="validate registry and configuration")
    parser.add_argument("--dry-run", action="store_true", help="resolve without writing")
    parser.add_argument("--force", action="store_true", help="refresh an exporter-generated output")
    args = parser.parse_args(argv)
    try:
        registry = read_json(args.registry)
        validate_registry(registry)
        if args.list:
            print_catalog(registry)
            return 0
        config = read_json(args.config)
        profile, requested, resolved, defines = resolve_config(registry, config)
        if args.check:
            print(f"configuration valid: {profile or 'custom'} -> {', '.join(resolved)}")
            return 0
        if args.dry_run:
            print(json.dumps({
                "profile": profile,
                "requested_modules": requested,
                "resolved_modules": resolved,
                "defines": defines,
            }, indent=2))
            return 0
        if not args.output:
            raise ConfigError("--output is required unless using --list, --check, or --dry-run")
        result = export_library(registry, config, args.output, args.force)
        print(
            f"exported {result['profile'] or 'custom'} to {result['output']} "
            f"({result['files']} files; {', '.join(result['resolved'])})"
        )
        return 0
    except ConfigError as error:
        parser.exit(2, f"flow32_configure: {error}\n")


if __name__ == "__main__":
    sys.exit(main())
