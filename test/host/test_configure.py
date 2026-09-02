#!/usr/bin/env python3

import copy
import json
import tempfile
import unittest
from pathlib import Path

import flow32_configure


class ConfigureTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.registry = flow32_configure.read_json(flow32_configure.DEFAULT_REGISTRY)

    def test_registry_is_complete(self):
        self.assertTrue(flow32_configure.validate_registry(self.registry))

    def test_retained_ui_closure(self):
        config = {"schema": 1, "profile": "retained-ui", "modules": [], "defines": {}}
        _, _, resolved, _ = flow32_configure.resolve_config(self.registry, config)
        self.assertIn("retained-ui", resolved)
        self.assertIn("display", resolved)
        self.assertNotIn("storage", resolved)
        self.assertNotIn("runtime", resolved)

    def test_full_profile_resolves_every_module(self):
        config = {"schema": 1, "profile": "full", "modules": [], "defines": {}}
        _, _, resolved, _ = flow32_configure.resolve_config(self.registry, config)
        self.assertEqual(set(resolved), set(self.registry["modules"]))

    def test_export_contains_only_selected_sources(self):
        config = {
            "schema": 1,
            "profile": "retained-ui",
            "modules": [],
            "defines": {"FLOW32_UI_ARENA_BYTES": 6144},
        }
        with tempfile.TemporaryDirectory(prefix="flow32-export-test-") as temporary:
            output = Path(temporary) / "Flow32"
            result = flow32_configure.export_library(self.registry, config, output)
            self.assertEqual(result["profile"], "retained-ui")
            self.assertTrue((output / "src/flow32/graphics/Canvas.cpp").is_file())
            self.assertTrue((output / "src/Flow32Selected.h").is_file())
            self.assertFalse((output / "src/flow32/assets/Storage.cpp").exists())
            self.assertFalse((output / "src/Flow32.h").exists())
            build_config = (output / "src/Flow32BuildConfig.h").read_text()
            self.assertIn("#define FLOW32_MODULE_RETAINED_UI 1", build_config)
            self.assertIn("#define FLOW32_MODULE_RUNTIME 0", build_config)
            self.assertIn("#define FLOW32_UI_ARENA_BYTES 6144", build_config)
            lock = json.loads((output / "flow32.lock.json").read_text())
            self.assertIn("retained-ui", lock["resolved_modules"])
            self.assertNotIn("runtime", lock["resolved_modules"])
            for relative, digest in lock["output_sha256"].items():
                self.assertEqual(flow32_configure.sha256(output / relative), digest)
            flow32_configure.export_library(
                self.registry, config, output, force=True
            )

    def test_force_refuses_unrelated_directory(self):
        config = {"schema": 1, "profile": "display-only", "modules": [], "defines": {}}
        with tempfile.TemporaryDirectory(prefix="flow32-export-test-") as temporary:
            output = Path(temporary) / "existing"
            output.mkdir()
            (output / "user-file.txt").write_text("keep")
            with self.assertRaises(flow32_configure.ConfigError):
                flow32_configure.export_library(
                    self.registry, config, output, force=True
                )
            self.assertEqual((output / "user-file.txt").read_text(), "keep")

    def test_cycle_and_define_injection_are_rejected(self):
        registry = copy.deepcopy(self.registry)
        registry["modules"]["foundation"]["requires"] = ["display"]
        with self.assertRaises(flow32_configure.ConfigError):
            flow32_configure.resolve_modules(registry, ["display"])
        config = {
            "schema": 1,
            "profile": "display-only",
            "modules": [],
            "defines": {"FLOW32_OK\n#error injected": 1},
        }
        with self.assertRaises(flow32_configure.ConfigError):
            flow32_configure.resolve_config(self.registry, config)


if __name__ == "__main__":
    unittest.main()
