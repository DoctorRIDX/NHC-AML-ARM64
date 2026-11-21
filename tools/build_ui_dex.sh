#!/bin/sh
# Java menu (java/) -> classes -> classes.dex -> generated/nhc_ui_dex.h
# Needs: JDK 17, ANDROID_HOME with a platform (android.jar) and build-tools (d8), python3.
# The GitHub Actions workflow runs this; you do not need it on your phone.
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
[ -n "$SDK" ] || { echo "error: ANDROID_HOME is not set" >&2; exit 1; }

PLATFORM=$(ls -d "$SDK"/platforms/android-* 2>/dev/null | sort -V | tail -n 1)
BT=$(ls -d "$SDK"/build-tools/* 2>/dev/null | sort -V | tail -n 1)
JAR="$PLATFORM/android.jar"
[ -f "$JAR" ] || { echo "error: android.jar not found under $SDK/platforms" >&2; exit 1; }
[ -x "$BT/d8" ] || { echo "error: d8 not found under $SDK/build-tools" >&2; exit 1; }
echo "Using $JAR and $BT/d8"

OUT="$ROOT/build/ui"
rm -rf "$OUT"
mkdir -p "$OUT/classes" "$OUT/dex" "$ROOT/generated"

# shellcheck disable=SC2046
javac -Xlint:-options -source 8 -target 8 -bootclasspath "$JAR" -d "$OUT/classes" $(find "$ROOT/java" -name '*.java')
# shellcheck disable=SC2046
"$BT/d8" --release --min-api 26 --lib "$JAR" --output "$OUT/dex" $(find "$OUT/classes" -name '*.class')
python3 "$ROOT/tools/embed_dex.py" "$OUT/dex/classes.dex" "$ROOT/generated/nhc_ui_dex.h"
