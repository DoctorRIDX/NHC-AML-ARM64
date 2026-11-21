#!/usr/bin/env python3
"""Fails when tools/nhc_offsets.py ENTRIES and kEntries in src/core.cpp drift apart."""
import importlib.util
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("nhc_offsets", root / "tools" / "nhc_offsets.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

src = (root / "src" / "core.cpp").read_text()
block = re.search(r"kEntries\[E_COUNT\] = \{(.*?)\n\};", src, re.S).group(1)
cpp = {}
for name, symbols in re.findall(r'\{"(\w+)", "[^"]*", "([^"]*)"\}', block):
    cpp[name] = [s for s in symbols.split(",") if s]

if cpp != mod.ENTRIES:
    print("MISMATCH\n C++   :", cpp, "\n Python:", mod.ENTRIES)
    sys.exit(1)
print("entries in sync (%d)" % len(cpp))
