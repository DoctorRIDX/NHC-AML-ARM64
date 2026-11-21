# LESSONS.md - failure ledger (format from the LiquidexGPT skill)

### L-001  Brace expansion silently failed in the build sandbox shell
- Project: NHC
- Date: 2026-11-20
- Symptom: `mkdir -p NHC/{a,b,c}` created one directory literally named `{a,b,c}`; later heredocs failed with "Directory nonexistent"
- Root cause: verified. The tool shell is `/bin/sh` (dash), which has no brace expansion
- Failed approach (fingerprint): `/bin/sh` + `{a,b}` brace expansion in mkdir
- Rule going forward: use an explicit `for d in a b c; do mkdir -p "$d"; done` or run `bash -c`
- Status: fixed

### L-002  The sandbox cannot compile Android code
- Project: NHC
- Date: 2026-11-20
- Symptom: no NDK, no `android.jar`, no `jni.h`, no network for bash
- Root cause: verified. Environment limit, not a code bug
- Failed approach (fingerprint): "build and run the APK/.so inside the chat sandbox"
- Rule going forward: logic is unit-tested on the PC with a fake platform; Android-facing files are only syntax-checked against hand-written API stubs; the real build is the GitHub Actions workflow. Never report the .so as built or tested
- Status: impossible-here (use CI)

### L-003  Game 2.11.311 has no public AML-compatible offset table
- Project: NHC
- Date: 2026-11-20
- Symptom: aml-psdk README lists only GTA SA 2.00 and 2.10 (64-bit)
- Root cause: verified (README). Offsets from other builds must not be reused
- Failed approach (fingerprint): copying offsets from another game build
- Rule going forward: features need a signature-checked table for the exact build (tools/nhc_offsets.py) or exported symbols; otherwise they stay NOT IMPLEMENTED
- Status: open

### L-004  Lambdas broke the DEX step in CI
- Project: NHC
- Date: 2026-11-21
- Symptom: `javac` error "cannot find symbol: method metafactory ... interface LambdaMetafactory" in NHCMenu.java, then d8 "Unable to find method metafactory"
- Root cause: verified from the log. The script picked `platforms/android-37.2-beta3/android.jar` (highest by `sort -V`), whose class set lacks `LambdaMetafactory.metafactory`; with `-source 8 -bootclasspath android.jar` every lambda needs it
- Why my local check missed it: I compiled against hand-written stubs with the JDK's own java.* classes, so lambdas always resolved
- Failed approach (fingerprint): lambdas + `-bootclasspath <newest android.jar>`
- Fix: no lambdas or method references in java/ (plain classes via one dispatcher), the script now rejects them before javac, and it prefers the newest stable `android-NN` platform
- Rule going forward: a local check that substitutes the real dependency proves nothing about that dependency; keep Java to plain classes
- Status: fixed (not yet confirmed by a new CI run)
