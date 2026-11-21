#!/usr/bin/env python3
"""NHC offset helper. Pure standard library, runs in Termux (pkg install python).

Works on the game's native library (lib/arm64-v8a/libGame.so, extracted from the APK with ZArchiver).

  nhc_offsets.py exports libGame.so [--grep Cheat]     list exported functions (rva + name)
  nhc_offsets.py sig     libGame.so 0x1A2B3C [-n 16]   print the first bytes at an rva
  nhc_offsets.py generate libGame.so --version-name 2.11.311 [--version-code N]
                          [--manual NAME=0xRVA ...] [--symbol NAME=mangled ...]
                                                       print [Build] and [Offsets_*] sections for nhc.ini

Only functions that really are in the library (exported by name, or given with --manual) are written,
each with the first bytes as signature. The plugin refuses an entry whose bytes do not match.
This tool never guesses an address.
"""
import argparse
import hashlib
import struct
import sys

# Keep in sync with kEntries in src/core.cpp (tests/check_entries_sync.py verifies it).
ENTRIES = {
    "FindPlayerPed": ["_Z13FindPlayerPedi"],
    "CGame_Process": ["_ZN5CGame7ProcessEv"],
    "CCheat_MoneyArmourHealthCheat": ["_ZN6CCheat22MoneyArmourHealthCheatEv"],
    "CCheat_JetpackCheat": ["_ZN6CCheat12JetpackCheatEv"],
    "CCheat_VehicleCheat": ["_ZN6CCheat12VehicleCheatEi", "_ZN6CCheat12VehicleCheatE8eModelID"],
    "CPed_GiveWeapon": ["_ZN4CPed10GiveWeaponE11eWeaponTypejb"],
    "CPed_InflictDamage": ["_ZN4CPed13InflictDamageEP7CEntity11eWeaponTypef14ePedPieceTypesh"],
}

PT_LOAD = 1
SHT_DYNSYM = 11
STT_FUNC = 2


class Elf:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        d = self.data
        if d[:4] != b"\x7fELF":
            raise SystemExit("error: not an ELF file")
        if d[4] != 2 or d[5] != 1:
            raise SystemExit("error: only 64-bit little-endian ELF is supported")
        (self.e_type, self.e_machine, _ver, _entry, self.phoff, self.shoff, _flags, _ehsize,
         self.phentsize, self.phnum, self.shentsize, self.shnum, _shstrndx) = struct.unpack_from("<HHIQQQIHHHHHH", d, 16)
        self.loads = []
        for i in range(self.phnum):
            p_type, _fl, p_off, p_vaddr, _pa, p_filesz, p_memsz, _al = struct.unpack_from(
                "<IIQQQQQQ", d, self.phoff + i * self.phentsize)
            if p_type == PT_LOAD:
                self.loads.append((p_vaddr, p_off, p_filesz))
        self.sections = []
        for i in range(self.shnum):
            self.sections.append(struct.unpack_from("<IIQQQQIIQQ", d, self.shoff + i * self.shentsize))

    def machine_name(self):
        return {183: "aarch64", 62: "x86_64", 40: "arm"}.get(self.e_machine, str(self.e_machine))

    def file_offset(self, vaddr):
        for p_vaddr, p_off, p_filesz in self.loads:
            if p_vaddr <= vaddr < p_vaddr + p_filesz:
                return p_off + (vaddr - p_vaddr)
        return None

    def read_at_rva(self, rva, n):
        off = self.file_offset(rva)
        if off is None or off + n > len(self.data):
            return None
        return self.data[off:off + n]

    def dynamic_functions(self):
        """name -> (rva, size) for defined, exported functions."""
        out = {}
        dynsym = [s for s in self.sections if s[1] == SHT_DYNSYM]
        if not dynsym:
            raise SystemExit("error: no .dynsym section (section headers are stripped); "
                             "exported symbols cannot be listed from this file")
        sh = dynsym[0]
        strtab = self.sections[sh[6]]
        count = sh[5] // 24
        for i in range(count):
            st_name, st_info, _other, st_shndx, st_value, st_size = struct.unpack_from(
                "<IBBHQQ", self.data, sh[4] + i * 24)
            if st_shndx == 0 or (st_info & 0xF) != STT_FUNC or st_name == 0:
                continue
            start = strtab[4] + st_name
            end = self.data.index(b"\0", start)
            out[self.data[start:end].decode("ascii", "replace")] = (st_value, st_size)
        return out


def parse_hex(text):
    return int(text, 16)


def cmd_exports(args):
    elf = Elf(args.lib)
    funcs = elf.dynamic_functions()
    shown = 0
    for name, (rva, size) in sorted(funcs.items(), key=lambda kv: kv[1][0]):
        if args.grep and args.grep.lower() not in name.lower():
            continue
        print("0x%08x %6d %s" % (rva, size, name))
        shown += 1
    print("-- %d of %d exported functions shown (machine: %s)" % (shown, len(funcs), elf.machine_name()), file=sys.stderr)
    return 0


def cmd_sig(args):
    elf = Elf(args.lib)
    raw = elf.read_at_rva(parse_hex(args.rva), args.n)
    if raw is None:
        print("error: rva is outside the file's loadable segments", file=sys.stderr)
        return 1
    print(raw.hex())
    return 0


def cmd_generate(args):
    elf = Elf(args.lib)
    if elf.e_machine != 183:
        print("warning: this library is %s, not aarch64" % elf.machine_name(), file=sys.stderr)
    with open(args.lib, "rb") as f:
        sha = hashlib.sha256(f.read()).hexdigest()
    print("library sha256: %s" % sha, file=sys.stderr)

    try:
        funcs = elf.dynamic_functions()
    except SystemExit as e:
        if not args.manual:
            raise
        print(str(e), file=sys.stderr)
        funcs = {}

    extra = {}
    for item in args.symbol or []:
        name, _, mangled = item.partition("=")
        extra.setdefault(name, []).append(mangled)
    manual = {}
    for item in args.manual or []:
        name, _, rva = item.partition("=")
        if name not in ENTRIES:
            raise SystemExit("error: unknown entry name %r (known: %s)" % (name, ", ".join(ENTRIES)))
        manual[name] = parse_hex(rva)

    lines = ["[Build]", "version_name=%s" % args.version_name]
    if args.version_code is not None:
        lines.append("version_code=%d" % args.version_code)
    if args.apk_md5:
        lines.append("apk_md5=%s" % args.apk_md5.lower())
    lines.append("")

    found, missing = [], []
    for name, candidates in ENTRIES.items():
        rva = manual.get(name)
        how = "manual"
        if rva is None:
            for cand in extra.get(name, []) + candidates:
                if cand in funcs:
                    rva, how = funcs[cand][0], "exported " + cand
                    break
        if rva is None:
            missing.append(name)
            continue
        raw = elf.read_at_rva(rva, 16)
        if raw is None:
            print("warning: %s rva 0x%x is outside the file, skipped" % (name, rva), file=sys.stderr)
            missing.append(name)
            continue
        lines += ["[Offsets_%s]" % name, "rva=0x%x" % rva, "sig=%s" % raw.hex(), ""]
        found.append("%s (%s)" % (name, how))

    print("\n".join(lines))
    print("found:   %s" % (", ".join(found) or "none"), file=sys.stderr)
    print("missing: %s" % (", ".join(missing) or "none"), file=sys.stderr)
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("exports")
    p.add_argument("lib")
    p.add_argument("--grep")
    p.set_defaults(fn=cmd_exports)

    p = sub.add_parser("sig")
    p.add_argument("lib")
    p.add_argument("rva")
    p.add_argument("-n", type=int, default=16)
    p.set_defaults(fn=cmd_sig)

    p = sub.add_parser("generate")
    p.add_argument("lib")
    p.add_argument("--version-name", required=True)
    p.add_argument("--version-code", type=int)
    p.add_argument("--apk-md5")
    p.add_argument("--manual", action="append", metavar="NAME=0xRVA")
    p.add_argument("--symbol", action="append", metavar="NAME=mangled")
    p.set_defaults(fn=cmd_generate)

    args = ap.parse_args()
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
